#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "core/ModuleRegistry.hpp"
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <mutex>
#include <functional>
#include <optional>

namespace PaperCrawler {

/**
 * @brief 系统资源使用情况
 */
struct SystemResources {
    double cpuUsagePercent{0.0};      // CPU使用率
    double memoryUsagePercent{0.0};   // 内存使用率
    uint64_t memoryTotal{0};          // 总内存（字节）
    uint64_t memoryUsed{0};           // 已用内存（字节）
    uint64_t memoryAvailable{0};      // 可用内存（字节）
    double diskUsagePercent{0.0};     // 磁盘使用率
    uint64_t diskTotal{0};            // 总磁盘空间（字节）
    uint64_t diskUsed{0};             // 已用磁盘空间（字节）
    uint64_t diskAvailable{0};        // 可用磁盘空间（字节）
    uint64_t networkRecvBytes{0};     // 网络接收字节数
    uint64_t networkSentBytes{0};     // 网络发送字节数
    int loadAverage1m{0};             // 1分钟负载平均值
    int loadAverage5m{0};             // 5分钟负载平均值
    int loadAverage15m{0};            // 15分钟负载平均值

    std::string toJSON() const;
};

/**
 * @brief 系统运行时间
 */
struct SystemUptime {
    uint64_t totalSeconds{0};
    uint64_t days{0};
    uint64_t hours{0};
    uint64_t minutes{0};
    uint64_t seconds{0};
    std::chrono::system_clock::time_point startTime;

    std::string toJSON() const;
    std::string format() const;
};

/**
 * @brief 系统信息
 */
struct SystemInfo {
    std::string hostname;
    std::string osType;
    std::string osVersion;
    std::string osArchitecture;
    std::string cpuModel;
    int cpuCores{0};
    uint64_t cpuFrequency{0};        // CPU频率（Hz）
    uint64_t totalMemory{0};         // 总内存（字节）
    std::string kernelVersion;
    std::string cppVersion;

    std::string toJSON() const;
};

/**
 * @brief 模块状态
 */
enum class ModuleStatus {
    UNLOADED,
    LOADED,
    STARTED,
    STOPPED,
    ERROR
};

/**
 * @brief 性能指标
 */
struct PerformanceMetrics {
    std::map<std::string, uint64_t> requestCounts;      // 各模块请求计数
    std::map<std::string, std::chrono::microseconds> averageResponseTimes;  // 平均响应时间
    std::map<std::string, double> throughput;           // 吞吐量（请求/秒）
    std::map<std::string, uint64_t> errorCounts;        // 错误计数
    std::map<std::string, double> p50Latency;           // P50延迟
    std::map<std::string, double> p95Latency;           // P95延迟
    std::map<std::string, double> p99Latency;           // P99延迟

    std::string toJSON() const;
};

/**
 * @brief 统计API模块
 *
 * 功能：
 * 1. 系统统计
 * 2. 模块统计
 * 3. 性能指标
 * 4. 资源监控
 * 5. 实时数据流
 *
 * 端点：
 * - GET /api/stats/system        - 系统信息
 * - GET /api/stats/resources     - 资源使用情况
 * - GET /api/stats/uptime        - 运行时间
 * - GET /api/stats/modules       - 模块状态
 * - GET /api/stats/modules/:name - 单个模块状态
 * - GET /api/stats/performance   - 性能指标
 * - GET /api/stats/realtime      - 实时数据流（SSE）
 */
class StatsApiModule : public BusinessModuleBase {
public:
    StatsApiModule();
    ~StatsApiModule() override;

    std::string getName() const override { return "StatsApi"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "System statistics and monitoring API";
    }

    /**
     * @brief 获取系统信息
     */
    SystemInfo getSystemInfo();

    /**
     * @brief 获取资源使用情况
     */
    SystemResources getResources();

    /**
     * @brief 获取运行时间
     */
    SystemUptime getUptime();

    /**
     * @brief 获取所有模块信息
     */
    std::vector<ModuleInfo> getAllModules();

    /**
     * @brief 获取指定模块信息
     */
    std::optional<ModuleInfo> getModule(const std::string& moduleName);

    /**
     * @brief 获取性能指标
     */
    PerformanceMetrics getPerformanceMetrics();

    /**
     * @brief 获取实时统计数据（JSON格式）
     */
    std::string getRealtimeStats();

    /**
     * @brief 启动实时数据流（Server-Sent Events）
     */
    std::string startRealtimeStream();

    /**
     * @brief 停止实时数据流
     */
    void stopRealtimeStream();

    /**
     * @brief 获取系统健康状态
     */
    bool isHealthy();

    /**
     * @brief 获取健康检查详情
     */
    std::map<std::string, bool> getHealthDetails();

    /**
     * @brief 更新性能指标
     */
    void updatePerformanceMetrics(const std::string& moduleName,
                                  uint64_t requestCount,
                                  std::chrono::microseconds responseTime,
                                  bool success);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void registerRoutes() override;
    std::string handleSystemInfo();
    std::string handleResources();
    std::string handleUptime();
    std::string handleModules();
    std::string handleModule(const std::string& moduleName);
    std::string handlePerformance();
    std::string handleRealtime();

    void monitorLoop();
    SystemInfo collectSystemInfo();
    SystemResources collectResources();

    std::chrono::system_clock::time_point startTime_;
    std::vector<std::thread> monitorThreads_;
    std::atomic<bool> streaming_{false};

    // 性能指标存储
    std::map<std::string, PerformanceMetrics> performanceHistory_;
    std::mutex mutex_;
};

} // namespace PaperCrawler
