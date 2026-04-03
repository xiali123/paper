#include "modules/TemplateCrawlerModule.hpp"
#include "data/IDatabase.hpp"
#include "data/DatabaseModule.hpp"
#include "spdlog/spdlog.h"
#include "json.hpp"
#include <sstream>
#include <regex>
#include <algorithm>
#include <chrono>
#include <iomanip>

// Gumbo Parser (HTML5 CSS选择器)
#include <gumbo.h>

// libxml2 (XPath) - 条件编译
#ifdef HAVE_LIBXML2
    #include <libxml/HTMLparser.h>
    #include <libxml/HTMLtree.h>
    #include <libxml/xpath.h>
    #include <libxml/tree.h>
    #include <libxml/parser.h>
    #include <libxml/xpathInternals.h>
#endif

namespace PaperCrawler {

// ============================================================================
// Implementation Class
// ============================================================================

class TemplateCrawlerModule::Impl {
public:
    // 解析缓存
    std::map<std::string, CrawlerTemplate> templateCache_;

    // 统计信息
    uint64_t totalCrawls_{0};
    uint64_t successfulCrawls_{0};
    uint64_t failedCrawls_{0};
};

// ============================================================================
// Constructor and Destructor
// ============================================================================

TemplateCrawlerModule::TemplateCrawlerModule(std::shared_ptr<IDatabase> database)
    : database_(database), impl_(std::make_unique<Impl>()) {

    // 初始化HTTP客户端
    httpClient_ = std::make_shared<Network::HttpClient>();
}

TemplateCrawlerModule::~TemplateCrawlerModule() = default;

// ============================================================================
// IModule Implementation
// ============================================================================

bool TemplateCrawlerModule::initialize() {
    auto logger = spdlog::get("TemplateCrawler");
    if (logger) {
        logger->info("Initializing TemplateCrawlerModule...");
    }

    // 加载预置模板到缓存
    loadPresetTemplates();

    return true;
}

bool TemplateCrawlerModule::start() {
    auto logger = spdlog::get("TemplateCrawler");
    if (logger) {
        logger->info("TemplateCrawlerModule started successfully");
    }

    return true;
}

bool TemplateCrawlerModule::stop() {
    auto logger = spdlog::get("TemplateCrawler");
    if (logger) {
        logger->info("TemplateCrawlerModule stopped");
    }

    return true;
}

void TemplateCrawlerModule::cleanup() {
    // 清理资源
    impl_->templateCache_.clear();
}

// ============================================================================
// Template Management
// ============================================================================

bool TemplateCrawlerModule::saveTemplate(const CrawlerTemplate& tmpl, int createdBy) {
    try {
        // 验证模板
        std::vector<std::string> errors;
        if (!tmpl.validate(errors)) {
            auto logging = spdlog::get("TemplateCrawler");
            if (logging) {
                logging->error("Template validation failed: " +
                    errors.empty() ? "Unknown error" : errors[0]);
            }
            return false;
        }

        // 保存到数据库
        std::ostringstream sql;
        sql << "INSERT INTO crawler_templates "
            << "(template_id, name, description, version, author, tags, "
            << "template_config, source_type, requires_js_rendering, "
            << "is_official, is_public, created_by) "
            << "VALUES ('"
            << tmpl.templateId << "', '"
            << tmpl.name << "', '"
            << tmpl.description << "', '"
            << tmpl.version << "', '"
            << tmpl.author << "', '"
            << tmpl.toJson() << "', '"  // tags as JSON
            << tmpl.toJson() << "', '"  // full config
            << static_cast<int>(tmpl.sourceType) << "', "
            << (tmpl.requiresJsRendering ? 1 : 0) << ", "
            << "FALSE, TRUE, "
            << createdBy << ")";

        if (database_->execute(sql.str())) {
            // 更新缓存
            impl_->templateCache_[tmpl.templateId] = tmpl;

            auto logging = spdlog::get("TemplateCrawler");
            if (logging) {
                logging->info("Template saved successfully: " + tmpl.templateId);
            }
            return true;
        }

    } catch (const std::exception& e) {
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("Failed to save template: " + std::string(e.what()));
        }
    }

    return false;
}

std::optional<CrawlerTemplate> TemplateCrawlerModule::loadTemplate(const std::string& templateId) {
    // 先从缓存查找
    auto it = impl_->templateCache_.find(templateId);
    if (it != impl_->templateCache_.end()) {
        return it->second;
    }

    // 从数据库加载
    try {
        std::ostringstream sql;
        sql << "SELECT template_config FROM crawler_templates "
            << "WHERE template_id = '" << templateId << "' "
            << "AND is_active = 1 "
            << "LIMIT 1";

        auto rows = database_->query(sql.str());
        if (!rows.empty()) {
            auto row = rows[0];
            std::string configJson = row.at("template_config");

            auto tmpl = CrawlerTemplate::fromJson(configJson);
            if (tmpl.has_value()) {
                // 缓存模板
                impl_->templateCache_[templateId] = tmpl.value();
                return tmpl;
            }
        }
    } catch (const std::exception& e) {
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("Failed to load template: " + std::string(e.what()));
        }
    }

    return std::nullopt;
}

bool TemplateCrawlerModule::deleteTemplate(const std::string& templateId) {
    try {
        std::ostringstream sql;
        sql << "DELETE FROM crawler_templates WHERE template_id = '"
            << templateId << "'";

        if (database_->execute(sql.str())) {
            // 从缓存中移除
            impl_->templateCache_.erase(templateId);

            auto logging = spdlog::get("TemplateCrawler");
            if (logging) {
                logging->info("Template deleted: " + templateId);
            }
            return true;
        }
    } catch (const std::exception& e) {
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("Failed to delete template: " + std::string(e.what()));
        }
    }

    return false;
}

std::vector<CrawlerTemplate> TemplateCrawlerModule::listTemplates(bool activeOnly) {
    std::vector<CrawlerTemplate> templates;

    try {
        std::ostringstream sql;
        sql << "SELECT template_id, template_config FROM crawler_templates";

        if (activeOnly) {
            sql << " WHERE is_active = 1";
        }

        auto rows = database_->query(sql.str());

        for (const auto& row : rows) {
            std::string configJson = row.at("template_config");
            auto tmpl = CrawlerTemplate::fromJson(configJson);
            if (tmpl.has_value()) {
                templates.push_back(tmpl.value());
            }
        }
    } catch (const std::exception& e) {
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("Failed to list templates: " + std::string(e.what()));
        }
    }

    return templates;
}

// ============================================================================
// Template Validation
// ============================================================================

TemplateValidationResult TemplateCrawlerModule::validateTemplate(const CrawlerTemplate& tmpl) {
    TemplateValidationResult result;
    result.isValid = true;

    // 1. 验证基本信息
    if (tmpl.templateId.empty()) {
        result.errors.push_back("template_id is required");
        result.isValid = false;
    }

    if (tmpl.name.empty()) {
        result.errors.push_back("name is required");
        result.isValid = false;
    }

    if (tmpl.baseUrl.empty()) {
        result.errors.push_back("baseUrl is required");
        result.isValid = false;
    }

    // 2. 验证URL格式
    std::string urlPattern = R"(^https?://[a-zA-Z0-9\-\.]+\.[a-zA-Z]{2,})";
    std::regex urlRegex(urlPattern);
    if (!std::regex_match(tmpl.baseUrl, urlRegex)) {
        result.errors.push_back("Invalid baseUrl format");
        result.isValid = false;
    }

    // 3. 验证字段规则
    if (tmpl.fieldRules.empty()) {
        result.errors.push_back("fieldRules cannot be empty");
        result.isValid = false;
    } else {
        for (const auto& [fieldName, rule] : tmpl.fieldRules) {
            // 验证必需字段有规则
            if (rule.required && rule.selector.empty()) {
                result.errors.push_back("Field " + fieldName + " is required but has no selector");
                result.isValid = false;
            }

            // 验证选择器/正则表达式
            if (rule.ruleType == RuleType::CSS_SELECTOR) {
                if (rule.selector.empty()) {
                    result.errors.push_back("CSS selector is empty for field: " + fieldName);
                    result.isValid = false;
                }
            } else if (rule.ruleType == RuleType::XPATH) {
                if (rule.selector.empty()) {
                    result.errors.push_back("XPath expression is empty for field: " + fieldName);
                    result.isValid = false;
                }
#ifdef HAVE_LIBXML2
                // 验证XPath语法
                xmlXPathContextPtr ctxt = xmlXPathNewContext(NULL);
                xmlXPathCompExprPtr comp = xmlXPathCompile(BAD_CAST(xmlChar*)(rule.selector.c_str()));
                if (comp == NULL) {
                    result.errors.push_back("Invalid XPath syntax for field: " + fieldName);
                    result.isValid = false;
                } else {
                    xmlXPathFreeCompExpr(comp);
                }
                xmlXPathFreeContext(ctxt);
#else
                auto logger = spdlog::get("TemplateCrawler");
                if (logger) {
                    logger->warn("XPath validation skipped - libxml2 not available");
                }
#endif
            } else if (rule.ruleType == RuleType::REGEX) {
                if (rule.selector.empty()) {
                    result.errors.push_back("Regex pattern is empty for field: " + fieldName);
                    result.isValid = false;
                }
                // 验证正则表达式语法
                try {
                    std::regex regex(rule.selector);
                } catch (const std::regex_error& e) {
                    result.errors.push_back("Invalid regex for field " + fieldName + ": " + e.what());
                    result.isValid = false;
                }
            } else if (rule.ruleType == RuleType::JSON_PATH) {
                if (rule.jsonPath.empty()) {
                    result.errors.push_back("JSONPath expression is empty for field: " + fieldName);
                    result.isValid = false;
                }
            }
        }
    }

    // 4. 验证分页配置
    if (tmpl.pagination.type.empty()) {
        result.warnings.push_back("Pagination type is not specified");
    }

    // 5. 验证速率限制
    if (tmpl.rateLimit.requestsPerMinute <= 0) {
        result.warnings.push_back("requestsPerMinute should be positive");
    }

    // 6. 验证JavaScript渲染配置
    if (tmpl.requiresJsRendering && tmpl.jsWaitTime <= 0) {
        result.warnings.push_back("jsWaitTime should be positive when requiresJsRendering is true");
    }

    return result;
}

// ============================================================================
// Core Crawling Interface
// ============================================================================

std::vector<CrawledPaper> TemplateCrawlerModule::crawlWithTemplate(
    const std::string& templateId,
    const std::map<std::string, std::string>& params) {

    std::vector<CrawledPaper> papers;
    impl_->totalCrawls_++;

    try {
        // 加载模板
        auto tmplOpt = loadTemplate(templateId);
        if (!tmplOpt.has_value()) {
            auto logging = spdlog::get("TemplateCrawler");
            if (logging) {
                logging->error("Template not found: " + templateId);
            }
            impl_->failedCrawls_++;
            return papers;
        }

        auto tmpl = tmplOpt.value();

        // 构建URL
        std::string url = buildUrl(tmpl, params);

        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->info("Crawling with template: " + templateId + " URL: " + url);
        }

        // 执行HTTP请求
        std::string response = executeRequest(tmpl, url, params);

        // 解析响应
        papers = parseResponse(tmpl, response, tmpl.sourceType);

        impl_->successfulCrawls_++;

        if (logging) {
            logging->info("Crawl completed. Found " + std::to_string(papers.size()) + " papers");
        }

    } catch (const std::exception& e) {
        impl_->failedCrawls_++;
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("Crawl failed: " + std::string(e.what()));
        }
    }

    return papers;
}

// ============================================================================
// Internal Parsing Methods
// ============================================================================

std::string TemplateCrawlerModule::parseWithCssSelector(
    const std::string& html,
    const FieldRule& rule) {

    // 使用Gumbo解析HTML
    GumboOutput* output = gumbo_parse(html.c_str());

    std::string result;

    // 查找匹配的元素
    // 注意：这里需要实现CSS选择器到Gumbo Node的匹配
    // 简化实现：直接搜索class或id
    // TODO: 实现完整的CSS选择器解析器

    gumbo_destroy_output(&kGumboDefaultOptions, &output[0]);

    return result;
}

std::string TemplateCrawlerModule::parseWithXPath(
    const std::string& html,
    const FieldRule& rule) {

#ifdef HAVE_LIBXML2
    std::string result;

    // 解析HTML为XML
    htmlDocPtr doc = htmlParseDoc(reinterpret_cast<const xmlChar*>(html.c_str()), NULL);
    if (doc == NULL) {
        return result;
    }

    // 创建XPath上下文
    xmlXPathContextPtr ctxt = xmlXPathNewContext(doc);
    if (ctxt == NULL) {
        xmlFreeDoc(doc);
        return result;
    }

    // 评估XPath表达式
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(
        BAD_CAST rule.selector.c_str(),
        ctxt
    );

    if (xpathObj != NULL) {
        if (xpathObj->type == XPATH_NODESET) {
            xmlNodeSetPtr nodes = xpathObj->nodesetval;

            if (nodes != NULL && nodes->nodeNr > 0) {
                xmlNodePtr node = nodes->nodeTab[0];

                // 提取文本内容或属性
                if (rule.attribute == "text") {
                    xmlChar* content = xmlNodeGetContent(node);
                    if (content != NULL) {
                        result = std::string(reinterpret_cast<char*>(content));
                        xmlFree(content);
                    }
                } else {
                    xmlChar* attr = xmlGetProp(
                        node,
                        BAD_CAST rule.attribute.c_str()
                    );
                    if (attr != NULL) {
                        result = std::string(reinterpret_cast<char*>(attr));
                        xmlFree(attr);
                    }
                }
            }
        }

        xmlXPathFreeObject(xpathObj);
    }

    xmlXPathFreeContext(ctxt);
    xmlFreeDoc(doc);

    return result;
#else
    auto logger = spdlog::get("TemplateCrawler");
    if (logger) {
        logger->warn("XPath support not compiled - install libxml2 to enable");
    }
    return "";
#endif
}

std::string TemplateCrawlerModule::parseWithRegex(
    const std::string& text,
    const FieldRule& rule) {

    std::string result;

    try {
        std::regex regex(rule.selector);
        std::smatch match;

        if (std::regex_search(text, match, regex)) {
            if (match.size() > 1) {
                result = match[1]; // 第一个捕获组
            } else {
                result = match[0]; // 整个匹配
            }
        }
    } catch (const std::regex_error& e) {
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("Regex parsing failed: " + std::string(e.what()));
        }
    }

    return result;
}

std::string TemplateCrawlerModule::parseWithJsonPath(
    const std::string& json,
    const FieldRule& rule) {

    std::string result;

    try {
        auto jsonDoc = nlohmann::json::parse(json);

        // 简化的JSONPath实现
        // TODO: 使用完整的JSONPath库（如jsonpath-cpp）

        // 例如：$.feed.entry -> jsonDoc["feed"]["entry"]
        std::string path = rule.jsonPath;
        if (path.substr(0, 2) == "$.") {
            path = path.substr(2); // 移除 "$."
        }

        // 分割路径
        std::vector<std::string> parts;
        std::stringstream ss(path);
        std::string part;
        while (std::getline(ss, part, '.')) {
            parts.push_back(part);
        }

        // 遍历路径
        nlohmann::json current = jsonDoc;
        for (const auto& p : parts) {
            if (current.contains(p)) {
                current = current[p];
            } else {
                return result; // 路径不存在
            }
        }

        // 提取结果
        if (current.is_string()) {
            result = current.get<std::string>();
        } else if (current.is_array()) {
            // 数组处理
            result = current.dump();
        }

    } catch (const std::exception& e) {
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("JSONPath parsing failed: " + std::string(e.what()));
        }
    }

    return result;
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string TemplateCrawlerModule::buildUrl(
    const CrawlerTemplate& tmpl,
    const std::map<std::string, std::string>& params) {

    std::string url = tmpl.baseUrl + tmpl.urlTemplate;

    // 替换URL模板中的参数
    for (const auto& [key, value] : params) {
        std::string placeholder = "{" + key + "}";
        size_t pos = url.find(placeholder);
        while (pos != std::string::npos) {
            url.replace(pos, placeholder.length(), value);
            pos = url.find(placeholder, pos + value.length());
        }
    }

    // 添加查询参数
    if (!params.empty()) {
        url += "?";
        bool first = true;
        for (const auto& [key, value] : params) {
            if (!first) url += "&";
            url += key + "=" + value;
            first = false;
        }
    }

    return url;
}

std::string TemplateCrawlerModule::executeRequest(
    const CrawlerTemplate& tmpl,
    const std::string& url,
    const std::map<std::string, std::string>& params) {

    auto logger = spdlog::get("TemplateCrawler");
    if (logger) {
        logger->debug("Executing request: " + url);
    }

    // 使用HTTP客户端发送请求
    Network::HttpClientResponse httpResponse = httpClient_->get(url);

    if (!httpResponse.isSuccess()) {
        throw std::runtime_error("HTTP request failed with status: " +
            std::to_string(httpResponse.statusCode));
    }

    return httpResponse.body;

    // TODO: 支持自定义headers和timeout
    // TODO: 支持POST方法
}


std::vector<CrawledPaper> TemplateCrawlerModule::parseResponse(
    const CrawlerTemplate& tmpl,
    const std::string& response,
    CrawlerSourceType sourceType) {

    std::vector<CrawledPaper> papers;

    // 根据数据源类型选择解析方式
    switch (sourceType) {
        case CrawlerSourceType::HTML:
            // HTML解析
            papers = parseHtmlResponse(tmpl, response);
            break;

        case CrawlerSourceType::API:
        case CrawlerSourceType::RSS:
            // JSON/XML解析
            papers = parseApiResponse(tmpl, response);
            break;

        default:
            throw std::runtime_error("Unsupported source type");
    }

    return papers;
}

std::vector<CrawledPaper> TemplateCrawlerModule::parseHtmlResponse(
    const CrawlerTemplate& tmpl,
    const std::string& html) {

    std::vector<CrawledPaper> papers;

    // TODO: 实现完整的HTML解析逻辑
    // 1. 使用Gumbo解析HTML
    // 2. 根据fieldRules提取数据
    // 3. 构建CrawledPaper对象

    return papers;
}

std::vector<CrawledPaper> TemplateCrawlerModule::parseApiResponse(
    const CrawlerTemplate& tmpl,
    const std::string& response) {

    std::vector<CrawledPaper> papers;

    try {
        auto jsonDoc = nlohmann::json::parse(response);

        // 提取论文列表
        // TODO: 根据fieldRules提取数据

    } catch (const std::exception& e) {
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("Failed to parse API response: " + std::string(e.what()));
        }
    }

    return papers;
}

std::string TemplateCrawlerModule::applyTransform(
    const std::string& value,
    const std::string& transform) {

    std::string result = value;

    if (transform == "trim") {
        // 去除首尾空格
        size_t start = result.find_first_not_of(" \t\n\r");
        size_t end = result.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
            result = result.substr(start, end - start + 1);
        }
    } else if (transform == "extract_year") {
        // 提取年份（假设格式：2023-01-01或2023）
        std::regex yearRegex(R"(\d{4})");
        std::smatch match;
        if (std::regex_search(value, match, yearRegex)) {
            result = match[1];
        }
    } else if (transform == "extract_number") {
        // 提取数字
        std::regex numRegex(R"(\d+)");
        std::smatch match;
        if (std::regex_search(value, match, numRegex)) {
            result = match[1];
        }
    } else if (transform == "to_lowercase") {
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    } else if (transform == "to_uppercase") {
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    }

    return result;
}

void TemplateCrawlerModule::loadPresetTemplates() {
    auto logger = spdlog::get("TemplateCrawler");
    if (logger) {
        logger->info("Loading preset templates...");
    }

    // 简化版本：只记录日志
    // 实际加载会在使用时通过数据库查询进行
    // TODO: 实现预置模板的预加载功能
}

void TemplateCrawlerModule::log(int taskId, const std::string& level, const std::string& message) {
    auto logging = spdlog::get("TemplateCrawler");
    if (logging) {
        if (level == "ERROR") {
            logging->error(message);
        } else if (level == "WARN") {
            logging->warn(message);
        } else if (level == "INFO") {
            logging->info(message);
        } else {
            logging->debug(message);
        }
    }
}

} // namespace PaperCrawler
