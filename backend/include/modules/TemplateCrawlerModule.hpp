#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include "modules/CrawlerModule.hpp"
#include "network/HttpClient.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <optional>

namespace PaperCrawler {

// ============================================================================
// 前向声明和类型别名
// ============================================================================

class IDatabase;

// 从CrawlerModule引入类型
using Modules::CrawlerSourceType;
using Modules::CrawledPaper;

// ============================================================================
// 模板系统数据结构
// ============================================================================

/**
 * @brief 解析规则类型
 */
enum class RuleType {
    CSS_SELECTOR,
    XPATH,
    REGEX,
    JSON_PATH
};

/**
 * @brief 字段解析规则
 */
struct BUSINESS_API FieldRule {
    RuleType ruleType;
    std::string selector;      // CSS选择器、XPath或正则表达式
    std::string jsonPath;      // JSONPath表达式
    std::string attribute;     // 提取属性（text、href等）
    bool isArray = false;      // 是否为数组
    bool required = false;     // 是否必需
    std::string transform;     // 转换函数名
    std::string separator;     // 数组分隔符
    std::string defaultValue;  // 默认值
};

/**
 * @brief 爬虫模板配置
 */
struct BUSINESS_API CrawlerTemplate {
    // 基本信息
    std::string templateId;
    std::string name;
    std::string description;
    std::string version;
    std::string author;
    std::vector<std::string> tags;

    // 数据源配置
    std::string baseUrl;
    std::string method;
    CrawlerSourceType sourceType;
    bool requiresJsRendering = false;
    int jsWaitTime = 0;
    std::string jsWaitForSelector;

    // URL模板
    std::string urlTemplate;
    std::string pageTemplate;

    // 请求配置
    std::map<std::string, std::string> headers;
    std::string contentType;
    int timeout = 30000;
    int retryCount = 3;
    int retryDelay = 1000;

    // 字段规则
    std::map<std::string, FieldRule> fieldRules;

    // 分页配置
    struct PaginationConfig {
        std::string type;  // PARAMETER, URL_PATTERN, HEADER
        std::string offsetParam;
        std::string limitParam;
        std::string pageParam;
        int maxLimit = 100;
        int defaultLimit = 20;
        int maxPages = 100;
    } pagination;

    // 认证配置
    struct AuthenticationConfig {
        std::string type;  // NONE, API_KEY, OAUTH, COOKIE
        std::map<std::string, std::string> authConfig;
    } authentication;

    // 速率限制
    struct RateLimitConfig {
        int requestsPerMinute = 60;
        int burstSize = 10;
    } rateLimit;

    // 序列化为JSON
    std::string toJson() const;

    // 从JSON解析
    static std::optional<CrawlerTemplate> fromJson(const std::string& json);

    // 验证模板
    bool validate(std::vector<std::string>& errors) const;
};

/**
 * @brief 模板验证结果
 */
struct BUSINESS_API TemplateValidationResult {
    bool isValid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::map<std::string, std::string> suggestions;
};

/**
 * @brief 模板测试结果
 */
struct BUSINESS_API TemplateTestResult {
    bool success;
    std::string testUrl;
    int papersFound;
    std::vector<CrawledPaper> samplePapers;
    std::string executionTime;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

// ============================================================================
// 模板爬虫模块
// ============================================================================

/**
 * @brief 模板化爬虫引擎
 *
 * 核心功能：
 * 1. 模板验证和测试
 * 2. 多种解析方式支持（CSS、XPath、正则、JSONPath）
 * 3. JavaScript渲染支持
 * 4. 数据转换Pipeline
 * 5. 错误处理和重试
 *
 * 使用示例：
 * ```cpp
 * TemplateCrawlerModule crawler(database);
 * auto result = crawler.validateTemplate(templateJson);
 * auto papers = crawler.crawlWithTemplate(templateId, queryParams);
 * ```
 */
class BUSINESS_API TemplateCrawlerModule : public IModule {
public:
    explicit TemplateCrawlerModule(std::shared_ptr<IDatabase> database);
    ~TemplateCrawlerModule() override;

    // IModule接口实现
    std::string getName() const override { return "TemplateCrawler"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Template-based web crawler engine";
    }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    // ========================================================================
    // 模板管理接口
    // ========================================================================

    /**
     * @brief 保存模板到数据库
     */
    bool saveTemplate(const CrawlerTemplate& tmpl, int createdBy);

    /**
     * @brief 从数据库加载模板
     */
    std::optional<CrawlerTemplate> loadTemplate(const std::string& templateId);

    /**
     * @brief 删除模板
     */
    bool deleteTemplate(const std::string& templateId);

    /**
     * @brief 列出所有模板
     */
    std::vector<CrawlerTemplate> listTemplates(bool activeOnly = true);

    /**
     * @brief 搜索模板
     */
    std::vector<CrawlerTemplate> searchTemplates(const std::string& keyword);

    // ========================================================================
    // 模板验证和测试
    // ========================================================================

    /**
     * @brief 验证模板
     */
    TemplateValidationResult validateTemplate(const CrawlerTemplate& tmpl);

    /**
     * @brief 验证JSON格式的模板
     */
    TemplateValidationResult validateTemplateJson(const std::string& json);

    /**
     * @brief 测试模板（实际爬取）
     */
    TemplateTestResult testTemplate(const std::string& templateId, const std::map<std::string, std::string>& params);

    /**
     * @brief 测试模板（使用提供的模板对象）
     */
    TemplateTestResult testTemplate(const CrawlerTemplate& tmpl, const std::map<std::string, std::string>& params);

    // ========================================================================
    // 核心爬取接口
    // ========================================================================

    /**
     * @brief 使用模板爬取数据
     */
    std::vector<CrawledPaper> crawlWithTemplate(
        const std::string& templateId,
        const std::map<std::string, std::string>& params
    );

    /**
     * @brief 使用模板爬取单条数据
     */
    std::optional<CrawledPaper> fetchPaperWithTemplate(
        const std::string& templateId,
        const std::string& paperId
    );

    /**
     * @brief 批量爬取
     */
    std::vector<CrawledPaper> batchCrawl(
        const std::string& templateId,
        const std::vector<std::map<std::string, std::string>>& paramsList
    );

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<Network::HttpClient> httpClient_;

    // ========================================================================
    // 内部解析方法
    // ========================================================================

    /**
     * @brief 使用CSS选择器解析
     */
    std::string parseWithCssSelector(
        const std::string& html,
        const FieldRule& rule
    );

    /**
     * @brief 使用XPath解析
     */
    std::string parseWithXPath(
        const std::string& html,
        const FieldRule& rule
    );

    /**
     * @brief 使用正则表达式解析
     */
    std::string parseWithRegex(
        const std::string& text,
        const FieldRule& rule
    );

    /**
     * @brief 使用JSONPath解析
     */
    std::string parseWithJsonPath(
        const std::string& json,
        const FieldRule& rule
    );

    /**
     * @brief 应用数据转换
     */
    std::string applyTransform(
        const std::string& value,
        const std::string& transform
    );

    /**
     * @brief 构建URL
     */
    std::string buildUrl(
        const CrawlerTemplate& tmpl,
        const std::map<std::string, std::string>& params
    );

    /**
     * @brief 执行HTTP请求
     */
    std::string executeRequest(
        const CrawlerTemplate& tmpl,
        const std::string& url,
        const std::map<std::string, std::string>& params
    );

    /**
     * @brief 解析响应
     */
    std::vector<CrawledPaper> parseResponse(
        const CrawlerTemplate& tmpl,
        const std::string& response,
        CrawlerSourceType sourceType
    );

    /**
     * @brief 处理JavaScript渲染
     */
    std::string renderJavaScript(const std::string& url, int waitTime);

    /**
     * @brief 加载预置模板
     */
    void loadPresetTemplates();

    /**
     * @brief 解析HTML响应
     */
    std::vector<CrawledPaper> parseHtmlResponse(
        const CrawlerTemplate& tmpl,
        const std::string& html
    );

    /**
     * @brief 解析API响应
     */
    std::vector<CrawledPaper> parseApiResponse(
        const CrawlerTemplate& tmpl,
        const std::string& response
    );

    /**
     * @brief 记录日志
     */
    void log(int taskId, const std::string& level, const std::string& message);
};

} // namespace PaperCrawler
