#pragma once

#include <string>
#include <map>
#include <any>
#include "ModuleExports.hpp"

namespace PaperCrawler {
namespace Messages {

// ============================================================================
// Module Messages - 类型安全的模块间通信消息
// ============================================================================

/**
 * @brief 模块调用消息
 */
struct ModuleCallMessage {
    std::string messageId;
    std::string targetModule;                           // 目标模块
    std::string method;                                 // 调用方法名
    std::map<std::string, std::string> parameters;     // 方法参数（类型安全）
    std::string sourceModule;
    int timeoutMs{5000};

    // 响应类型
    using ResponseType = struct {
        bool success;
        std::string result;                             // JSON格式的结果
        std::string errorMessage;
    };
};

/**
 * @brief 模块事件通知消息
 */
struct ModuleEventMessage {
    std::string messageId;
    std::string eventType;                              // 事件类型
    std::string eventData;                              // 事件数据（JSON格式）
    std::string sourceModule;
    std::string targetModule;                           // 目标模块（空表示广播）
    int priority{0};                                    // 优先级

    // 无需响应
    using ResponseType = void;
};

/**
 * @brief 模块状态查询消息
 */
struct ModuleStatusMessage {
    std::string messageId;
    std::string targetModule;                           // 目标模块（空查询所有）
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        std::string moduleName;
        std::string version;
        std::string state;                              // INITIALIZED, STARTED, STOPPED
        std::chrono::system_clock::time_point startTime;
        uint64_t processedRequests;
        uint64_t errorCount;
        std::map<std::string, std::string> customMetrics;
    };
};

/**
 * @brief 模块健康检查消息
 */
struct ModuleHealthCheckMessage {
    std::string messageId;
    std::string targetModule;
    std::string sourceModule;
    int timeoutMs{3000};

    // 响应类型
    using ResponseType = struct {
        bool healthy;
        std::string status;
        std::map<std::string, std::string> details;
    };
};

/**
 * @brief 模块配置更新消息
 */
struct ModuleConfigUpdateMessage {
    std::string messageId;
    std::string targetModule;
    std::map<std::string, std::string> configChanges;  // 配置变更
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        bool success;
        std::string errorMessage;
        std::vector<std::string> appliedChanges;
    };
};

/**
 * @brief 模块重载消息（热重载）
 */
struct ModuleReloadMessage {
    std::string messageId;
    std::string targetModule;
    std::string sourceModule;
    bool force{false};                                  // 强制重载

    // 响应类型
    using ResponseType = struct {
        bool success;
        std::string errorMessage;
        std::string oldVersion;
        std::string newVersion;
    };
};

} // namespace Messages
} // namespace PaperCrawler
