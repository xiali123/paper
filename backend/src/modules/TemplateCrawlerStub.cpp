#include "modules/TemplateCrawlerModule.hpp"
#include "../../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>

namespace PaperCrawler {

/**
 * @brief Stub implementation of CrawlerTemplate::toJson()
 *
 * This is a minimal stub that provides only the toJson() serialization method
 * for CrawlerTemplate, without the full TemplateCrawlerModule implementation
 * (which depends on gumbo HTML parser).
 *
 * This allows DistributedTaskModule to compile without requiring gumbo.
 */
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
        j["sourceType"] = sourceType;
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
            fieldRuleJson["ruleType"] = rule.ruleType;
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
        auto logging = spdlog::get("TemplateCrawler");
        if (logging) {
            logging->error("Failed to serialize CrawlerTemplate to JSON: {}", e.what());
        }
        return "{}";
    }
}

} // namespace PaperCrawler
