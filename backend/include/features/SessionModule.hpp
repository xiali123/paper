#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <map>
#include <optional>
#include <any>
#include <chrono>
#include <vector>
#include <mutex>

namespace PaperCrawler {

/**
 * @brief 会话数据
 */
struct Session {
    std::string sessionId;
    std::string userId;
    std::string ipAddress;
    std::string userAgent;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastAccessedAt;
    std::chrono::system_clock::time_point expiresAt;
    std::map<std::string, std::any> data;
    bool active{true};

    /**
     * @brief 检查会话是否过期
     */
    bool isExpired() const {
        return std::chrono::system_clock::now() >= expiresAt;
    }

    /**
     * @brief 获取会话剩余时间
     */
    std::chrono::seconds getTimeRemaining() const {
        auto now = std::chrono::system_clock::now();
        if (now >= expiresAt) {
            return std::chrono::seconds(0);
        }
        return std::chrono::duration_cast<std::chrono::seconds>(expiresAt - now);
    }
};

/**
 * @brief 会话创建选项
 */
struct SessionOptions {
    std::chrono::seconds ttl{3600};        // 默认1小时
    bool persistent{false};                // 是否持久化
    bool secure{true};                     // 是否只通过HTTPS传输
    bool httpOnly{true};                   // 是否禁止JavaScript访问
    std::string sameSite{"Strict"};        // SameSite策略
};

/**
 * @brief 会话统计
 */
struct SessionStats {
    size_t totalSessions{0};
    size_t activeSessions{0};
    size_t expiredSessions{0};
    std::chrono::system_clock::time_point lastCleanup;
    std::map<std::string, size_t> sessionsByUser;
};

/**
 * @brief 会话管理模块
 *
 * 功能：
 * 1. 分布式会话存储（Redis/内存）
 * 2. 会话过期管理
 * 3. 会话CRUD操作
 * 4. Cookie管理
 * 5. 会话预热
 * 6. 批量操作
 *
 * 特性：
 * - 自动过期：TTL管理
 * - 线程安全：互斥锁保护
 * - 高性能：内存缓存 + Redis持久化
 * - 分布式：支持多服务器共享会话
 */
class SessionModule : public IModule {
public:
    SessionModule();
    ~SessionModule() override;

    std::string getName() const override { return "Session"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Distributed session management with Redis backend";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 创建会话
     * @param userId 用户ID
     * @param options 会话选项
     * @return 会话ID
     */
    std::string createSession(const std::string& userId,
                             const SessionOptions& options = SessionOptions());

    /**
     * @brief 获取会话
     */
    std::optional<Session> getSession(const std::string& sessionId);

    /**
     * @brief 更新会话
     */
    bool updateSession(const std::string& sessionId, const Session& session);

    /**
     * @brief 删除会话
     */
    bool deleteSession(const std::string& sessionId);

    /**
     * @brief 刷新会话过期时间
     * @param additionalTTL 额外的TTL时间
     */
    bool refreshSession(const std::string& sessionId,
                       std::chrono::seconds additionalTTL = std::chrono::seconds(0));

    /**
     * @brief 检查会话是否存在且有效
     */
    bool isValidSession(const std::string& sessionId);

    /**
     * @brief 获取会话数据
     */
    template<typename T>
    std::optional<T> getSessionData(const std::string& sessionId, const std::string& key) {
        auto session = getSession(sessionId);
        if (!session.has_value()) {
            return std::nullopt;
        }

        auto it = session->data.find(key);
        if (it != session->data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (...) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief 设置会话数据
     */
    bool setSessionData(const std::string& sessionId, const std::string& key, const std::any& value);

    /**
     * @brief 删除会话数据
     */
    bool removeSessionData(const std::string& sessionId, const std::string& key);

    /**
     * @brief 获取用户的所有会话
     */
    std::vector<Session> getUserSessions(const std::string& userId);

    /**
     * @brief 删除用户的所有会话
     */
    size_t deleteUserSessions(const std::string& userId);

    /**
     * @brief 清理过期会话
     */
    size_t cleanupExpired();

    /**
     * @brief 清理所有会话
     */
    void clearAll();

    /**
     * @brief 获取会话统计
     */
    SessionStats getStats() const;

    /**
     * @brief 设置默认TTL
     */
    void setDefaultTTL(std::chrono::seconds ttl);

    /**
     * @brief 设置存储类型（memory/redis）
     */
    void setStorageType(const std::string& type);

    /**
     * @brief 生成会话ID
     */
    std::string generateSessionId();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    SessionOptions defaultOptions_;
    std::chrono::seconds defaultTTL_{3600};
    std::string storageType_{"memory"};  // memory or redis

    mutable std::mutex mutex_;
};

} // namespace PaperCrawler
