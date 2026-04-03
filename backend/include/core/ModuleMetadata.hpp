#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include "ModuleExports.hpp"

namespace PaperCrawler {

/**
 * @brief 模块健康状态
 */
enum class ModuleHealthStatus {
    HEALTHY,           // 模块正常工作
    DEGRADED,          // 模块功能降级
    UNHEALTHY,         // 模块不健康
    FAILED             // 模块失败
};

/**
 * @brief 模块依赖信息
 */
struct ModuleDependency {
    std::string moduleName;       // 依赖的模块名
    std::string minVersion;       // 最低版本要求
    bool optional;                // 是否可选

    ModuleDependency(const std::string& name, const std::string& version = "1.0.0", bool opt = false)
        : moduleName(name), minVersion(version), optional(opt) {}
};

/**
 * @brief 模块元数据
 *
 * 包含模块的所有必要信息，用于自动加载和路由注册
 */
struct ModuleMetadata {
    // 基本信息
    std::string name;                    // 模块名称（如 "AuthApi"）
    std::string version;                 // 模块版本（如 "1.0.0"）
    std::string description;             // 模块描述
    ModuleType type;                     // 模块类型（SERVER/BUSINESS）
    std::string author;                  // 作者
    std::string license;                 // 许可证

    // 路由信息（仅BUSINESS模块）
    std::string routePrefix;             // 路由前缀（如 "/api/auth"）
    std::vector<std::string> endpoints;  // 端点列表（如 ["POST /login", "GET /logout"]）

    // 依赖信息
    std::vector<ModuleDependency> dependencies;  // 依赖的其他模块
    int loadPriority;                     // 加载优先级（0-100，数字越大越优先加载）

    // 配置信息
    std::string configPath;               // 配置文件路径（可选）
    std::map<std::string, std::string> config;  // 配置键值对

    // 运行时信息
    std::string libraryPath;              // DLL/SO文件路径
    ModuleHandle handle;                  // 模块句柄
    ModuleHealthStatus healthStatus;      // 健康状态
    std::chrono::system_clock::time_point lastHealthCheck;  // 最后健康检查时间
    int failureCount;                     // 失败计数
    std::string lastError;                // 最后错误信息

    // 统计信息
    std::chrono::system_clock::time_point loadTime;        // 加载时间
    std::chrono::system_clock::time_point startTime;       // 启动时间
    size_t requestCount;                  // 请求计数
    size_t errorCount;                    // 错误计数

    // 构造函数
    ModuleMetadata()
        : type(ModuleType::SERVER)
        , loadPriority(50)
        , handle(nullptr)
        , healthStatus(ModuleHealthStatus::HEALTHY)
        , failureCount(0)
        , requestCount(0)
        , errorCount(0) {}

    /**
     * @brief 检查模块是否健康
     */
    bool isHealthy() const {
        return healthStatus == ModuleHealthStatus::HEALTHY;
    }

    /**
     * @brief 检查模块是否为业务模块
     */
    bool isBusinessModule() const {
        return type == ModuleType::BUSINESS;
    }

    /**
     * @brief 获取运行时间（秒）
     */
    long getUptimeSeconds() const {
        if (startTime == std::chrono::system_clock::time_point{}) {
            return 0;
        }
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
    }

    /**
     * @brief 获取错误率
     */
    double getErrorRate() const {
        if (requestCount == 0) return 0.0;
        return static_cast<double>(errorCount) / static_cast<double>(requestCount);
    }

    /**
     * @brief 更新健康状态
     */
    void updateHealthStatus(ModuleHealthStatus status, const std::string& errorMsg = "") {
        healthStatus = status;
        lastHealthCheck = std::chrono::system_clock::now();

        if (status == ModuleHealthStatus::FAILED || status == ModuleHealthStatus::UNHEALTHY) {
            failureCount++;
            lastError = errorMsg;
        } else {
            failureCount = 0;
            lastError.clear();
        }
    }

    /**
     * @brief 转换为JSON字符串
     */
    std::string toJson() const {
        std::ostringstream json;
        json << "{\n";
        json << "  \"name\": \"" << name << "\",\n";
        json << "  \"version\": \"" << version << "\",\n";
        json << "  \"description\": \"" << description << "\",\n";
        json << "  \"type\": \"" << (type == ModuleType::BUSINESS ? "BUSINESS" : "SERVER") << "\",\n";
        json << "  \"author\": \"" << author << "\",\n";
        json << "  \"license\": \"" << license << "\",\n";

        if (type == ModuleType::BUSINESS) {
            json << "  \"routePrefix\": \"" << routePrefix << "\",\n";
            json << "  \"endpoints\": [";
            for (size_t i = 0; i < endpoints.size(); ++i) {
                if (i > 0) json << ", ";
                json << "\"" << endpoints[i] << "\"";
            }
            json << "],\n";
        }

        json << "  \"loadPriority\": " << loadPriority << ",\n";
        json << "  \"healthStatus\": \"";
        switch (healthStatus) {
            case ModuleHealthStatus::HEALTHY: json << "HEALTHY"; break;
            case ModuleHealthStatus::DEGRADED: json << "DEGRADED"; break;
            case ModuleHealthStatus::UNHEALTHY: json << "UNHEALTHY"; break;
            case ModuleHealthStatus::FAILED: json << "FAILED"; break;
        }
        json << "\",\n";

        json << "  \"failureCount\": " << failureCount << ",\n";
        json << "  \"uptimeSeconds\": " << getUptimeSeconds() << ",\n";
        json << "  \"requestCount\": " << requestCount << ",\n";
        json << "  \"errorCount\": " << errorCount << ",\n";
        json << "  \"errorRate\": " << std::fixed << std::setprecision(2) << getErrorRate() << "\n";
        json << "}";
        return json.str();
    }
};

} // namespace PaperCrawler
