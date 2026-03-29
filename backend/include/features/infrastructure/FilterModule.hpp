#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <any>

namespace PaperCrawler {

/**
 * @brief 过滤器优先级
 */
enum class FilterPriority {
    CRITICAL = 0,  // 最高优先级（如安全检查）
    HIGH = 1,      // 高优先级（如认证）
    MEDIUM = 2,    // 中优先级（如限流）
    LOW = 3,       // 低优先级（如日志）
    BULK = 4       // 批量操作优先级
};

/**
 * @brief 过滤器结果
 */
enum class FilterResult {
    ACCEPT,    // 接受请求，继续处理
    REJECT,    // 拒绝请求
    MODIFY     // 修改请求后继续
};

/**
 * @brief HTTP请求上下文
 */
struct RequestContext {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query;
    std::string body;
    std::string clientIP;
    std::string userAgent;

    // 元数据
    std::map<std::string, std::any> metadata;
};

/**
 * @brief 过滤器接口
 */
class IFilterRule {
public:
    virtual ~IFilterRule() = default;

    /**
     * @brief 执行过滤
     */
    virtual FilterResult process(RequestContext& context) = 0;

    /**
     * @brief 获取过滤器名称
     */
    virtual std::string getName() const = 0;

    /**
     * @brief 获取优先级
     */
    virtual FilterPriority getPriority() const = 0;

    /**
     * @brief 是否启用
     */
    virtual bool isEnabled() const = 0;
};

/**
 * @brief 过滤器模块
 *
 * 功能：
 * 1. 管理过滤器链
 * 2. 按优先级执行过滤器
 * 3. IP黑白名单
 * 4. 速率限制
 * 5. 认证过滤
 * 6. 输入验证
 */
class FilterModule : public IModule {
public:
    FilterModule();
    ~FilterModule() override;

    std::string getName() const override { return "Filter"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Request filter chain manager";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }
    std::string getRoutePrefix() const override { return "/api/filters"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 注册过滤器
     */
    void registerFilter(std::shared_ptr<IFilterRule> filter);

    /**
     * @brief 注销过滤器
     */
    bool unregisterFilter(const std::string& name);

    /**
     * @brief 获取过滤器
     */
    std::shared_ptr<IFilterRule> getFilter(const std::string& name);

    /**
     * @brief 获取所有过滤器
     */
    std::vector<std::shared_ptr<IFilterRule>> getAllFilters();

    /**
     * @brief 处理请求（通过过滤器链）
     */
    FilterResult processRequest(RequestContext& context);

    /**
     * @brief 启用/禁用过滤器
     */
    bool setFilterEnabled(const std::string& name, bool enabled);

    /**
     * @brief 获取过滤器统计
     */
    struct FilterStats {
        std::string name;
        uint64_t totalRequests{0};
        uint64_t acceptedRequests{0};
        uint64_t rejectedRequests{0};
        uint64_t modifiedRequests{0};
        double averageProcessingTime{0.0};
    };
    std::vector<FilterStats> getFilterStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes();
    std::string handleList();
    std::string handleStats();
};

} // namespace PaperCrawler
