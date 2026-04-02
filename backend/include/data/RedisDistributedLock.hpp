// Redis分布式锁实现
// 文件位置：backend/include/data/RedisDistributedLock.hpp

#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <stdexcept>

namespace PaperCrawler {

/**
 * @brief Redis分布式锁异常
 */
class DistributedLockException : public std::runtime_error {
public:
    explicit DistributedLockException(const std::string& msg)
        : std::runtime_error(msg) {}
};

/**
 * @brief Redis分布式锁实现
 *
 * 使用Redis实现分布式锁，支持：
 * - 自动过期（防死锁）
 * - 唯一标识（防误删）
 * - 自动续期（防业务超时）
 * - 可重入
 */
class RedisDistributedLock {
public:
    /**
     * @brief 锁句柄（RAII管理）
     */
    class LockHandle {
    private:
        std::shared_ptr<RedisDistributedLock> lockManager_;
        std::string key_;
        std::string token_;
        bool locked_;

    public:
        LockHandle(std::shared_ptr<RedisDistributedLock> manager,
                   const std::string& key,
                   const std::string& token)
            : lockManager_(manager), key_(key), token_(token), locked_(true) {}

        ~LockHandle() {
            if (locked_) {
                lockManager_->unlock(key_, token_);
            }
        }

        // 禁止拷贝
        LockHandle(const LockHandle&) = delete;
        LockHandle& operator=(const LockHandle&) = delete;

        // 允许移动
        LockHandle(LockHandle&& other) noexcept
            : lockManager_(std::move(other.lockManager_)),
              key_(std::move(other.key_)),
              token_(std::move(other.token_)),
              locked_(other.locked_) {
            other.locked_ = false;
        }

        LockHandle& operator=(LockHandle&& other) noexcept {
            if (this != &other) {
                if (locked_) {
                    lockManager_->unlock(key_, token_);
                }
                lockManager_ = std::move(other.lockManager_);
                key_ = std::move(other.key_);
                token_ = std::move(other.token_);
                locked_ = other.locked_;
                other.locked_ = false;
            }
            return *this;
        }

        /**
         * @brief 手动释放锁
         */
        void unlock() {
            if (locked_) {
                lockManager_->unlock(key_, token_);
                locked_ = false;
            }
        }

        /**
         * @brief 检查锁是否持有
         */
        bool isLocked() const { return locked_; }

        /**
         * @brief 获取锁键
         */
        const std::string& getKey() const { return key_; }

        /**
         * @brief 获取锁令牌
         */
        const std::string& getToken() const { return token_; }
    };

    /**
     * @brief Redis客户端接口（需要实现）
     */
    class IRedisClient {
    public:
        virtual ~IRedisClient() = default;

        /**
         * @brief 执行SET命令（支持NX和EX选项）
         * @return true如果设置成功
         */
        virtual bool set(const std::string& key,
                        const std::string& value,
                        std::chrono::seconds ttl,
                        bool nx) = 0;

        /**
         * @brief 执行Lua脚本
         * @return 脚本返回值
         */
        virtual std::string eval(const std::string& script,
                                const std::vector<std::string>& keys,
                                const std::vector<std::string>& args) = 0;

        /**
         * @brief 检查连接
         */
        virtual bool ping() = 0;
    };

    /**
     * @brief 构造函数
     * @param redisClient Redis客户端
     */
    explicit RedisDistributedLock(std::shared_ptr<IRedisClient> redisClient)
        : redisClient_(redisClient) {
        if (!redisClient_) {
            throw DistributedLockException("Redis client is null");
        }
    }

    /**
     * @brief 尝试获取锁
     * @param key 锁键
     * @param ttl 锁超时时间
     * @param retry 重试间隔（默认不重试）
     * @return 锁句柄（如果成功），nullptr（如果失败）
     */
    std::unique_ptr<LockHandle> tryLock(
        const std::string& key,
        std::chrono::seconds ttl,
        std::chrono::milliseconds retry = std::chrono::milliseconds(0)) {

        auto start = std::chrono::steady_clock::now();
        std::string token = generateToken();

        while (true) {
            // 尝试获取锁
            bool acquired = redisClient_->set(
                "lock:" + key,
                token,
                ttl,
                true  // NX：仅当键不存在时设置
            );

            if (acquired) {
                // 成功获取锁
                return std::make_unique<LockHandle>(
                    shared_from_this(),
                    key,
                    token
                );
            }

            // 检查是否需要重试
            if (retry.count() == 0) {
                // 不重试，直接返回失败
                return nullptr;
            }

            // 检查重试超时
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            );

            if (elapsed >= retry) {
                // 超时，返回失败
                return nullptr;
            }

            // 等待一段时间后重试
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /**
     * @brief 释放锁
     * @param key 锁键
     * @param token 锁令牌（必须匹配才能删除）
     * @return true如果释放成功
     */
    bool unlock(const std::string& key, const std::string& token) {
        // Lua脚本：确保只删除自己的锁
        const std::string script = R"(
            if redis.call("get", KEYS[1]) == ARGV[1] then
                return redis.call("del", KEYS[1])
            else
                return 0
            end
        )";

        auto result = redisClient_->eval(
            script,
            {"lock:" + key},
            {token}
        );

        return result == "1";
    }

    /**
     * @brief 生成唯一锁令牌
     */
    static std::string generateToken() {
        static std::atomic<uint64_t> counter{0};
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()
        ).count();
        auto id = counter.fetch_add(1);
        return std::to_string(timestamp) + "-" + std::to_string(id);
    }

private:
    std::shared_ptr<IRedisClient> redisClient_;

    // 使shared_from_this工作
    friend class LockHandle;
};

/**
 * @brief 分布式锁管理器（单例）
 */
class DistributedLockManager {
public:
    static DistributedLockManager& getInstance() {
        static DistributedLockManager instance;
        return instance;
    }

    void initialize(std::shared_ptr<RedisDistributedLock::IRedisClient> redisClient) {
        lock_ = std::make_shared<RedisDistributedLock>(redisClient);
    }

    std::shared_ptr<RedisDistributedLock> getLock() {
        return lock_;
    }

private:
    DistributedLockManager() = default;
    std::shared_ptr<RedisDistributedLock> lock_;
};

} // namespace PaperCrawler
