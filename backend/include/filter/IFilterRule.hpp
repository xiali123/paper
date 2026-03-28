#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace PaperCrawler {

// 前向声明
struct HttpRequest;
struct HttpResponse;

/**
 * @brief 过滤器优先级
 */
enum class FilterPriority {
    CRITICAL = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    BULK = 4
};

/**
 * @brief 过滤器接口
 */
class IFilterRule {
public:
    virtual ~IFilterRule() = default;

    /**
     * @brief 过滤请求
     * @return {continue, errorMessage}
     */
    virtual std::pair<bool, std::string> filter(const HttpRequest& request) = 0;

    /**
     * @brief 获取优先级
     */
    virtual FilterPriority getPriority() const = 0;

    /**
     * @brief 获取过滤器名称
     */
    virtual std::string getName() const = 0;
};

/**
 * @brief 中间件管道
 */
class MiddlewarePipeline {
public:
    MiddlewarePipeline() = default;
    ~MiddlewarePipeline() = default;

    /**
     * @brief 添加过滤器
     */
    void addFilter(std::shared_ptr<IFilterRule> filter);

    /**
     * @brief 执行管道
     */
    std::pair<bool, std::string> execute(const HttpRequest& request);

    /**
     * @brief 获取所有过滤器
     */
    const std::vector<std::shared_ptr<IFilterRule>>& getFilters() const {
        return filters_;
    }

private:
    std::vector<std::shared_ptr<IFilterRule>> filters_;
};

/**
 * @brief 过滤器模块
 *
 * 管理所有请求过滤器：
 * 1. IP过滤（白名单/黑名单）
 * 2. 认证过滤
 * 3. 速率限制
 * 4. 输入验证
 * 5. 日志记录
 */
class FilterModule : public IModule {
public:
    FilterModule();
    ~FilterModule() override;

    // IModule接口实现
    std::string getName() const override { return "Filter"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Request filter chain manager";
    }
    ModuleType getModuleType() const override {
        return ModuleType::SERVER;
    }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 获取管道
     */
    MiddlewarePipeline& getPipeline() { return pipeline_; }

    /**
     * @brief 注册过滤器
     */
    void registerFilter(std::shared_ptr<IFilterRule> filter);

    /**
     * @brief 注销过滤器
     */
    void unregisterFilter(const std::string& filterName);

private:
    MiddlewarePipeline pipeline_;
};

} // namespace PaperCrawler
