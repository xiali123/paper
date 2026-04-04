#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include "ModuleExports.hpp"

namespace PaperCrawler {
namespace Messages {

// ============================================================================
// Cache Messages - 类型安全的缓存操作消息
// ============================================================================

/**
 * @brief 缓存获取消息
 */
struct CacheGetMessage {
    std::string messageId;
    std::string key;                                    // 缓存键
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        bool found;
        std::string value;
        std::chrono::system_clock::time_point expiry;
    };
};

/**
 * @brief 缓存设置消息
 */
struct CacheSetMessage {
    std::string messageId;
    std::string key;                                    // 缓存键
    std::string value;                                  // 缓存值
    std::optional<std::chrono::seconds> ttl;            // 过期时间（可选）
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        bool success;
        std::string errorMessage;
    };
};

/**
 * @brief 缓存删除消息
 */
struct CacheDeleteMessage {
    std::string messageId;
    std::string key;                                    // 缓存键
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        bool success;
        bool existed;
        std::string errorMessage;
    };
};

/**
 * @brief 批量缓存获取消息
 */
struct CacheBatchGetMessage {
    std::string messageId;
    std::vector<std::string> keys;                     // 批量键
    std::string sourceModule;

    // 响应类型
    using ResponseType = std::map<std::string, std::string>;
};

/**
 * @brief 批量缓存设置消息
 */
struct CacheBatchSetMessage {
    std::string messageId;
    std::map<std::string, std::string> keyValues;      // 键值对
    std::optional<std::chrono::seconds> ttl;            // 过期时间（可选）
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        bool success;
        size_t setCount;
        std::string errorMessage;
    };
};

/**
 * @brief 缓存清除消息
 */
struct CacheClearMessage {
    std::string messageId;
    std::string pattern;                                // 通配符模式（如"user:*"）
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        bool success;
        size_t clearedCount;
        std::string errorMessage;
    };
};

/**
 * @brief 缓存存在性检查消息
 */
struct CacheExistsMessage {
    std::string messageId;
    std::string key;                                    // 缓存键
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        bool exists;
        std::chrono::system_clock::time_point expiry;
    };
};

/**
 * @brief 缓存统计消息
 */
struct CacheStatsMessage {
    std::string messageId;
    std::string sourceModule;

    // 响应类型
    using ResponseType = struct {
        size_t totalKeys;
        size_t hitCount;
        size_t missCount;
        double hitRate;
        size_t memoryUsedBytes;
    };
};

} // namespace Messages
} // namespace PaperCrawler
