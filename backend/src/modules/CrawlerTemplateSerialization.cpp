#include "modules/TemplateCrawlerModule.hpp"
#include "../../../core/external/nlohmann/json.hpp"
#include <sstream>

namespace PaperCrawler {

// ============================================================================
// CrawlerTemplate 序列化实现
// ============================================================================

std::string CrawlerTemplate::toJson() const {
    try {
        nlohmann::json j;

        // 基本信息
        j["templateId"] = templateId;
        j["name"] = name;
        j["description"] = description;
        j["version"] = version;
        j["author"] = author;
        j["tags"] = tags;

        // 数据源配置
        j["baseUrl"] = baseUrl;
        j["method"] = method;
        j["sourceType"] = static_cast<int>(sourceType);
        j["requiresJsRendering"] = requiresJsRendering;
        j["jsWaitTime"] = jsWaitTime;
        j["jsWaitForSelector"] = jsWaitForSelector;

        // URL模板
        j["urlTemplate"] = urlTemplate;
        j["pageTemplate"] = pageTemplate;

        // 请求配置
        j["headers"] = nlohmann::json::object();
        for (const auto& [key, value] : headers) {
            j["headers"][key] = value;
        }
        j["contentType"] = contentType;
        j["timeout"] = timeout;
        j["retryCount"] = retryCount;
        j["retryDelay"] = retryDelay;

        // 字段规则
        j["fieldRules"] = nlohmann::json::object();
        for (const auto& [fieldName, rule] : fieldRules) {
            nlohmann::json fieldRuleJson;
            fieldRuleJson["ruleType"] = static_cast<int>(rule.ruleType);
            fieldRuleJson["selector"] = rule.selector;
            fieldRuleJson["jsonPath"] = rule.jsonPath;
            fieldRuleJson["attribute"] = rule.attribute;
            fieldRuleJson["isArray"] = rule.isArray;
            fieldRuleJson["required"] = rule.required;
            fieldRuleJson["transform"] = rule.transform;
            fieldRuleJson["separator"] = rule.separator;
            fieldRuleJson["defaultValue"] = rule.defaultValue;
            j["fieldRules"][fieldName] = fieldRuleJson;
        }

        // 分页配置
        j["pagination"]["type"] = pagination.type;
        j["pagination"]["offsetParam"] = pagination.offsetParam;
        j["pagination"]["limitParam"] = pagination.limitParam;
        j["pagination"]["pageParam"] = pagination.pageParam;
        j["pagination"]["maxLimit"] = pagination.maxLimit;
        j["pagination"]["defaultLimit"] = pagination.defaultLimit;
        j["pagination"]["maxPages"] = pagination.maxPages;

        // 认证配置
        j["authentication"]["type"] = authentication.type;
        j["authentication"]["authConfig"] = nlohmann::json::object();
        for (const auto& [key, value] : authentication.authConfig) {
            j["authentication"]["authConfig"][key] = value;
        }

        // 速率限制
        j["rateLimit"]["requestsPerMinute"] = rateLimit.requestsPerMinute;
        j["rateLimit"]["burstSize"] = rateLimit.burstSize;

        return j.dump();

    } catch (const std::exception& e) {
        return "{}";
    }
}

std::optional<CrawlerTemplate> CrawlerTemplate::fromJson(const std::string& jsonStr) {
    try {
        auto j = nlohmann::json::parse(jsonStr);

        CrawlerTemplate tmpl;

        // 基本信息
        if (j.contains("templateId")) j["templateId"].get_to(tmpl.templateId);
        if (j.contains("name")) j["name"].get_to(tmpl.name);
        if (j.contains("description")) j["description"].get_to(tmpl.description);
        if (j.contains("version")) j["version"].get_to(tmpl.version);
        if (j.contains("author")) j["author"].get_to(tmpl.author);
        if (j.contains("tags")) j["tags"].get_to(tmpl.tags);

        // 数据源配置
        if (j.contains("baseUrl")) j["baseUrl"].get_to(tmpl.baseUrl);
        if (j.contains("method")) j["method"].get_to(tmpl.method);
        if (j.contains("sourceType")) tmpl.sourceType = static_cast<CrawlerSourceType>(j["sourceType"].get<int>());
        if (j.contains("requiresJsRendering")) j["requiresJsRendering"].get_to(tmpl.requiresJsRendering);
        if (j.contains("jsWaitTime")) j["jsWaitTime"].get_to(tmpl.jsWaitTime);
        if (j.contains("jsWaitForSelector")) j["jsWaitForSelector"].get_to(tmpl.jsWaitForSelector);

        // URL模板
        if (j.contains("urlTemplate")) j["urlTemplate"].get_to(tmpl.urlTemplate);
        if (j.contains("pageTemplate")) j["pageTemplate"].get_to(tmpl.pageTemplate);

        // 请求配置
        if (j.contains("headers") && j["headers"].is_object()) {
            for (auto& [key, value] : j["headers"].items()) {
                tmpl.headers[key] = value.get<std::string>();
            }
        }
        if (j.contains("contentType")) j["contentType"].get_to(tmpl.contentType);
        if (j.contains("timeout")) j["timeout"].get_to(tmpl.timeout);
        if (j.contains("retryCount")) j["retryCount"].get_to(tmpl.retryCount);
        if (j.contains("retryDelay")) j["retryDelay"].get_to(tmpl.retryDelay);

        // 字段规则
        if (j.contains("fieldRules") && j["fieldRules"].is_object()) {
            for (auto& [fieldName, fieldRuleJson] : j["fieldRules"].items()) {
                FieldRule rule;
                if (fieldRuleJson.contains("ruleType")) rule.ruleType = static_cast<RuleType>(fieldRuleJson["ruleType"].get<int>());
                if (fieldRuleJson.contains("selector")) fieldRuleJson["selector"].get_to(rule.selector);
                if (fieldRuleJson.contains("jsonPath")) fieldRuleJson["jsonPath"].get_to(rule.jsonPath);
                if (fieldRuleJson.contains("attribute")) fieldRuleJson["attribute"].get_to(rule.attribute);
                if (fieldRuleJson.contains("isArray")) fieldRuleJson["isArray"].get_to(rule.isArray);
                if (fieldRuleJson.contains("required")) fieldRuleJson["required"].get_to(rule.required);
                if (fieldRuleJson.contains("transform")) fieldRuleJson["transform"].get_to(rule.transform);
                if (fieldRuleJson.contains("separator")) fieldRuleJson["separator"].get_to(rule.separator);
                if (fieldRuleJson.contains("defaultValue")) fieldRuleJson["defaultValue"].get_to(rule.defaultValue);
                tmpl.fieldRules[fieldName] = rule;
            }
        }

        // 分页配置
        if (j.contains("pagination") && j["pagination"].is_object()) {
            auto& pagJson = j["pagination"];
            if (pagJson.contains("type")) pagJson["type"].get_to(tmpl.pagination.type);
            if (pagJson.contains("offsetParam")) pagJson["offsetParam"].get_to(tmpl.pagination.offsetParam);
            if (pagJson.contains("limitParam")) pagJson["limitParam"].get_to(tmpl.pagination.limitParam);
            if (pagJson.contains("pageParam")) pagJson["pageParam"].get_to(tmpl.pagination.pageParam);
            if (pagJson.contains("maxLimit")) pagJson["maxLimit"].get_to(tmpl.pagination.maxLimit);
            if (pagJson.contains("defaultLimit")) pagJson["defaultLimit"].get_to(tmpl.pagination.defaultLimit);
            if (pagJson.contains("maxPages")) pagJson["maxPages"].get_to(tmpl.pagination.maxPages);
        }

        // 认证配置
        if (j.contains("authentication") && j["authentication"].is_object()) {
            auto& authJson = j["authentication"];
            if (authJson.contains("type")) authJson["type"].get_to(tmpl.authentication.type);
            if (authJson.contains("authConfig") && authJson["authConfig"].is_object()) {
                for (auto& [key, value] : authJson["authConfig"].items()) {
                    tmpl.authentication.authConfig[key] = value.get<std::string>();
                }
            }
        }

        // 速率限制
        if (j.contains("rateLimit") && j["rateLimit"].is_object()) {
            auto& rateJson = j["rateLimit"];
            if (rateJson.contains("requestsPerMinute")) rateJson["requestsPerMinute"].get_to(tmpl.rateLimit.requestsPerMinute);
            if (rateJson.contains("burstSize")) rateJson["burstSize"].get_to(tmpl.rateLimit.burstSize);
        }

        return tmpl;

    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

bool CrawlerTemplate::validate(std::vector<std::string>& errors) const {
    errors.clear();
    bool isValid = true;

    // 基本字段验证
    if (templateId.empty()) {
        errors.push_back("templateId is required");
        isValid = false;
    }

    if (name.empty()) {
        errors.push_back("name is required");
        isValid = false;
    }

    if (baseUrl.empty()) {
        errors.push_back("baseUrl is required");
        isValid = false;
    }

    if (method.empty()) {
        errors.push_back("method is required");
        isValid = false;
    }

    // URL模板验证
    if (urlTemplate.empty()) {
        errors.push_back("urlTemplate is required");
        isValid = false;
    }

    // 字段规则验证
    if (fieldRules.empty()) {
        errors.push_back("at least one field rule is required");
        isValid = false;
    }

    // 分页配置验证
    if (pagination.type.empty()) {
        errors.push_back("pagination type is required");
        isValid = false;
    }

    return isValid;
}

} // namespace PaperCrawler
