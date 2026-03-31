#pragma once

#include "core/IModule.hpp"
#include "core/Router.hpp"
#include "core/ModuleExports.hpp"
#include <map>
#include <memory>
#include <functional>
#include <atomic>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace PaperCrawler {

/**
 * @brief 服务器模块基类
 *
 * 为系统级服务器模块提供通用功能：
 * - 生命周期管理
 * - 状态跟踪
 * - 性能监控
 * - 健康检查
 * - 配置管理
 *
 * 适用模块：DatabaseModule, CacheModule, HttpServerModule等
 */
class ServerModuleBase : public IModule {
public:
    virtual ~ServerModuleBase() = default;

    /**
     * @brief 初始化模块（带模板方法模式）
     */
    bool initialize() override {
        if (state_ != ModuleState::UNLOADED) {
            return false;
        }

        startTime_ = std::chrono::system_clock::now();

        // 调用子类的具体初始化
        bool success = onInitialize();

        if (success) {
            state_ = ModuleState::INITIALIZED;
            metrics_["initialized_at"] = getCurrentTimestamp();
        }

        return success;
    }

    /**
     * @brief 启动模块（带模板方法模式）
     */
    bool start() override {
        if (state_ != ModuleState::INITIALIZED) {
            return false;
        }

        // 调用子类的具体启动
        bool success = onStart();

        if (success) {
            state_ = ModuleState::STARTED;
            metrics_["started_at"] = getCurrentTimestamp();
        }

        return success;
    }

    /**
     * @brief 停止模块（带模板方法模式）
     */
    bool stop() override {
        if (state_ != ModuleState::STARTED) {
            return false;
        }

        // 调用子类的具体停止
        bool success = onStop();

        state_ = ModuleState::STOPPED;
        metrics_["stopped_at"] = getCurrentTimestamp();

        return success;
    }

    /**
     * @brief 清理资源（带模板方法模式）
     */
    void cleanup() override {
        // 调用子类的具体清理
        onCleanup();

        state_ = ModuleState::UNLOADED;
        metrics_.clear();
    }

    /**
     * @brief 健康检查
     */
    virtual bool isHealthy() const {
        return state_ == ModuleState::STARTED;
    }

    /**
     * @brief 获取模块指标
     */
    std::map<std::string, std::string> getMetrics() const {
        auto metrics = metrics_;

        // 添加运行时指标
        metrics["state"] = moduleStateToString(state_);
        metrics["uptime_seconds"] = std::to_string(getUptimeSeconds());
        metrics["processed_requests"] = std::to_string(processedRequests_);

        return metrics;
    }

    /**
     * @brief 更新模块指标
     */
    void setMetric(const std::string& key, const std::string& value) {
        metrics_[key] = value;
    }

    /**
     * @brief 增加请求计数
     */
    void incrementRequestCount() {
        processedRequests_++;
    }

    /**
     * @brief 增加错误计数
     */
    void incrementErrorCount() {
        errorCount_++;
    }

    /**
     * @brief 获取运行时间（秒）
     */
    uint64_t getUptimeSeconds() const {
        if (state_ != ModuleState::STARTED) {
            return 0;
        }

        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_);
        return duration.count();
    }

protected:
    /**
     * @brief 子类实现具体的初始化逻辑
     */
    virtual bool onInitialize() = 0;

    /**
     * @brief 子类实现具体的启动逻辑
     */
    virtual bool onStart() = 0;

    /**
     * @brief 子类实现具体的停止逻辑
     */
    virtual bool onStop() = 0;

    /**
     * @brief 子类实现具体的清理逻辑
     */
    virtual void onCleanup() = 0;

    /**
     * @brief 获取当前时间戳字符串
     */
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    /**
     * @brief 模块状态转字符串
     */
    static std::string moduleStateToString(ModuleState state) {
        switch (state) {
            case ModuleState::UNLOADED: return "UNLOADED";
            case ModuleState::LOADED: return "LOADED";
            case ModuleState::INITIALIZED: return "INITIALIZED";
            case ModuleState::STARTED: return "STARTED";
            case ModuleState::STOPPED: return "STOPPED";
            case ModuleState::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    // 运行时指标
    std::map<std::string, std::string> metrics_;
    std::chrono::system_clock::time_point startTime_;
    std::atomic<uint64_t> processedRequests_{0};
    std::atomic<uint64_t> errorCount_{0};
};

/**
 * @brief 业务模块基类
 *
 * 为业务API模块提供通用功能：
 * - 路由自动注册
 * - 请求/响应处理
 * - 中间件支持
 * - 认证/授权检查
 * - 请求日志
 *
 * 适用模块：PaperApiModule, AuthApiModule, UserApiModule等
 */
class BusinessModuleBase : public IModule {
public:
    virtual ~BusinessModuleBase() = default;

    /**
     * @brief 获取模块类型（固定为BUSINESS）
     */
    ModuleType getModuleType() const override {
        return ModuleType::BUSINESS;
    }

    /**
     * @brief 初始化并注册路由
     */
    bool initialize() override {
        if (state_ != ModuleState::UNLOADED) {
            return false;
        }

        // 注册路由
        registerRoutes();

        state_ = ModuleState::INITIALIZED;
        return true;
    }

    /**
     * @brief 启动模块
     */
    bool start() override {
        if (state_ != ModuleState::INITIALIZED) {
            return false;
        }

        state_ = ModuleState::STARTED;
        return true;
    }

    /**
     * @brief 停止模块
     */
    bool stop() override {
        if (state_ != ModuleState::STARTED) {
            return false;
        }

        state_ = ModuleState::STOPPED;
        return true;
    }

    /**
     * @brief 清理资源
     */
    void cleanup() override {
        routes_.clear();
        state_ = ModuleState::UNLOADED;
    }

    /**
     * @brief 注册路由（子类必须实现）
     */
    virtual void registerRoutes() = 0;

    /**
     * @brief 获取注册的路由
     */
    const std::map<std::string, RouteHandler>& getRoutes() const {
        return routes_;
    }

    /**
     * @brief 添加路由
     */
    void addRoute(const std::string& path, RouteHandler handler) {
        routes_[path] = handler;
    }

    /**
     * @brief 处理请求（带中间件）
     */
    HttpResponse handleRequest(const HttpRequest& req) {
        // 前置中间件
        for (auto& middleware : beforeMiddlewares_) {
            auto result = middleware(req);
            if (result.statusCode != 0) {
                return result;  // 中间件拦截了请求
            }
        }

        // 查找路由处理器
        auto path = req.path;
        auto it = routes_.find(path);

        if (it == routes_.end()) {
            HttpResponse notFound;
            notFound.statusCode = 404;
            notFound.body = R"({"error":"Not Found","message":"Route not found: )" + path + R"("})";
            return notFound;
        }

        // 执行路由处理器
        HttpResponse response = it->second(req);

        // 后置中间件
        for (auto& middleware : afterMiddlewares_) {
            middleware(response);
        }

        return response;
    }

    /**
     * @brief 添加前置中间件
     */
    void addBeforeMiddleware(std::function<HttpResponse(const HttpRequest&)> middleware) {
        beforeMiddlewares_.push_back(middleware);
    }

    /**
     * @brief 添加后置中间件
     */
    void addAfterMiddleware(std::function<void(HttpResponse&)> middleware) {
        afterMiddlewares_.push_back(middleware);
    }

    /**
     * @brief 添加认证中间件
     */
    void addAuthMiddleware(std::function<bool(const HttpRequest&)> authCheck) {
        addBeforeMiddleware([authCheck](const HttpRequest& req) -> HttpResponse {
            if (!authCheck(req)) {
                HttpResponse unauthorized;
                unauthorized.statusCode = 401;
                unauthorized.body = R"({"error":"Unauthorized","message":"Authentication required"})";
                unauthorized.setHeader("Content-Type", "application/json");
                return unauthorized;
            }

            // 返回statusCode=0表示继续处理
            HttpResponse continue_;
            continue_.statusCode = 0;
            return continue_;
        });
    }

protected:
    std::map<std::string, RouteHandler> routes_;
    std::vector<std::function<HttpResponse(const HttpRequest&)>> beforeMiddlewares_;
    std::vector<std::function<void(HttpResponse&)>> afterMiddlewares_;
};

} // namespace PaperCrawler
