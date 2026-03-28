#pragma once

#include "framework/IModule.hpp"
#include "framework/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 负载均衡策略
 */
enum class LoadBalanceStrategy {
    ROUND_ROBIN,      // 轮询
    LEAST_CONNECTIONS,// 最少连接
    IP_HASH,         // IP哈希
    RANDOM,          // 随机
    WEIGHTED_ROUND_ROBIN,  // 加权轮询
    LEAST_RESPONSE_TIME    // 最短响应时间
};

/**
 * @brief 健康检查状态
 */
enum class HealthStatus {
    HEALTHY,
    UNHEALTHY,
    DRAINING,     // 正在排空（不接收新请求）
    DISABLED      // 已禁用
};

/**
 * @brief 上游服务器
 */
struct UpstreamServer {
    std::string id;
    std::string host;
    int port{80};
    int weight{1};                // 权重（用于加权轮询）
    HealthStatus healthStatus{HealthStatus::HEALTHY};

    // 健康检查
    std::chrono::system_clock::time_point lastHealthCheck;
    std::chrono::system_clock::time_point lastSuccessTime;
    std::chrono::system_clock::time_point lastFailureTime;
    uint64_t consecutiveFailures{0};
    uint64_t consecutiveSuccesses{0};

    // 统计信息
    uint64_t totalRequests{0};
    uint64_t successfulRequests{0};
    uint64_t failedRequests{0};
    std::chrono::microseconds totalResponseTime{0};
    std::chrono::microseconds averageResponseTime{0};

    // SSL配置
    bool enableSSL{false};
    std::string sslVerify;  // none, peer, strict

    // 元数据
    std::map<std::string, std::string> metadata;
    std::map<std::string, std::string> headers;  // 自定义请求头

    /**
     * @brief 获取失败率
     */
    double getFailureRate() const {
        if (totalRequests == 0) return 0.0;
        return static_cast<double>(failedRequests) / totalRequests;
    }

    /**
     * @brief 是否健康
     */
    bool isHealthy() const {
        return healthStatus == HealthStatus::HEALTHY;
    }
};

/**
 * @brief 健康检查配置
 */
struct HealthCheckConfig {
    bool enabled{true};
    std::chrono::seconds interval{10};     // 检查间隔
    std::chrono::seconds timeout{5};       // 超时时间
    std::string path{"/health"};           // 健康检查路径
    std::string method{"GET"};             // HTTP方法
    int unhealthyThreshold{3};             // 不健康阈值（连续失败次数）
    int healthyThreshold{2};               // 健康阈值（连续成功次数）
    std::map<std::string, std::string> headers;  // 自定义请求头
    std::string expectedBody;              // 期望的响应体
    int expectedStatus{200};               // 期望的状态码
};

/**
 * @brief 代理配置
 */
struct ProxyConfig {
    std::string listenAddress{"0.0.0.0"};
    int listenPort{8080};
    LoadBalanceStrategy loadBalanceStrategy{LoadBalanceStrategy::ROUND_ROBIN};
    bool enableHealthCheck{true};
    bool enableRetry{true};
    int maxRetries{3};
    std::chrono::seconds retryTimeout{30};
    bool enableCaching{false};
    size_t cacheSize{1000000};  // 缓存大小（字节）
    std::chrono::seconds cacheTTL{300};  // 缓存TTL
    bool enableCompression{false};
    bool enableAccessLog{true};
    std::string accessLogPath{"./logs/proxy_access.log"};
    bool enableErrorLog{true};
    std::string errorLogPath{"./logs/proxy_error.log"};
    std::chrono::seconds clientTimeout{60};
    std::chrono::seconds upstreamTimeout{60};
};

/**
 * @brief 代理统计
 */
struct ProxyStats {
    uint64_t totalRequests{0};
    uint64_t successfulRequests{0};
    uint64_t failedRequests{0};
    uint64_t retriesAttempted{0};
    uint64_t cacheHits{0};
    uint64_t cacheMisses{0};
    std::chrono::microseconds averageResponseTime{0};
    std::map<std::string, uint64_t> requestsByUpstream;
    std::map<int, uint64_t> statusCodes;  // 状态码分布
    std::chrono::system_clock::time_point lastRequestTime;
};

/**
 * @brief 代理请求
 */
struct ProxyRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string queryString;
    std::string clientIP;

    // 元数据
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief 代理响应
 */
struct ProxyResponse {
    int statusCode{200};
    std::map<std::string, std::string> headers;
    std::string body;
    std::chrono::microseconds responseTime{0};
    std::string upstreamServerId;
    bool fromCache{false};
};

/**
 * @brief 代理模块
 *
 * 功能：
 * 1. 反向代理
 * 2. 负载均衡
 * 3. 健康检查
 * 4. 重试机制
 * 5. 缓存
 * 6. SSL/TLS终止
 * 7. 访问日志
 *
 * 特性：
 * - 高可用：自动故障转移
 * - 高性能：连接池，缓存
 * - 可观测：访问日志，统计
 * - 灵活：多种负载均衡策略
 * - 安全：SSL/TLS支持
 */
class ProxyModule : public IModule {
public:
    ProxyModule();
    ~ProxyModule() override;

    std::string getName() const override { return "Proxy"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Reverse proxy with load balancing and health checks";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 添加上游服务器
     */
    bool addUpstream(const UpstreamServer& server);

    /**
     * @brief 移除上游服务器
     */
    bool removeUpstream(const std::string& serverId);

    /**
     * @brief 获取上游服务器
     */
    std::optional<UpstreamServer> getUpstream(const std::string& serverId) const;

    /**
     * @brief 获取所有上游服务器
     */
    std::vector<UpstreamServer> getAllUpstreams() const;

    /**
     * @brief 获取健康的服务器
     */
    std::vector<UpstreamServer> getHealthyUpstreams() const;

    /**
     * @brief 选择服务器（负载均衡）
     */
    std::optional<UpstreamServer> selectServer();

    /**
     * @brief 设置负载均衡策略
     */
    void setLoadBalanceStrategy(LoadBalanceStrategy strategy);

    /**
     * @brief 获取负载均衡策略
     */
    LoadBalanceStrategy getLoadBalanceStrategy() const { return config_.loadBalanceStrategy; }

    /**
     * @brief 健康检查
     */
    void healthCheck();

    /**
     * @brief 手动触发健康检查
     */
    bool manualHealthCheck(const std::string& serverId);

    /**
     * @brief 禁用服务器
     */
    bool disableServer(const std::string& serverId);

    /**
     * @brief 启用服务器
     */
    bool enableServer(const std::string& serverId);

    /**
     * @brief 设置服务器权重
     */
    bool setServerWeight(const std::string& serverId, int weight);

    /**
     * @brief 代理请求
     */
    ProxyResponse proxyRequest(const ProxyRequest& request);

    /**
     * @brief 获取代理统计
     */
    ProxyStats getStats() const;

    /**
     * @brief 设置配置
     */
    void setConfig(const ProxyConfig& config);

    /**
     * @brief 设置健康检查配置
     */
    void setHealthCheckConfig(const HealthCheckConfig& config);

    /**
     * @brief 清空缓存
     */
    void clearCache();

    /**
     * @brief 获取缓存统计
     */
    struct CacheStats {
        size_t size;
        size_t count;
        uint64_t hits;
        uint64_t misses;
        double hitRate;
    };
    CacheStats getCacheStats() const;

    /**
     * @brief 设置请求/响应修改器
     */
    using RequestModifier = std::function<void(ProxyRequest&)>;
    using ResponseModifier = std::function<void(ProxyResponse&)>;

    void setRequestModifier(RequestModifier modifier);
    void setResponseModifier(ResponseModifier modifier);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void healthCheckLoop();
    void updateServerHealth(const std::string& serverId, bool healthy);
    UpstreamServer* selectServerByRoundRobin();
    UpstreamServer* selectServerByLeastConnections();
    UpstreamServer* selectServerByIPHash(const std::string& clientIP);
    UpstreamServer* selectServerByRandom();
    UpstreamServer* selectServerByWeightedRoundRobin();
    UpstreamServer* selectServerByLeastResponseTime();
    std::string generateServerId(const std::string& host, int port);

    ProxyConfig config_;
    HealthCheckConfig healthCheckConfig_;
    std::vector<UpstreamServer> upstreams_;
    std::map<std::string, UpstreamServer*> upstreamMap_;

    // 负载均衡状态
    size_t roundRobinIndex_{0};
    std::atomic<uint64_t> requestCounter_{0};

    RequestModifier requestModifier_;
    ResponseModifier responseModifier_;

    ProxyStats stats_;
    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
