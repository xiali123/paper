#pragma once

#include <string>
#include <memory>
#include <optional>
#include <map>
#include <vector>

#ifdef USE_REDIS_CACHE
#include <hiredis/hiredis.h>
#endif

namespace PaperCrawler {

/**
 * @brief Redis连接管理类
 *
 * 提供Redis基本操作，支持：
 * - 基本CRUD操作（set/get/del/exists）
 * - TTL管理（expire/ttl）
 * - 批量操作（mset/mget）
 * - 高级操作（incr/keys/flushAll）
 * - Pipeline支持
 */
class RedisConnection {
public:
    /**
     * @brief 构造函数
     * @param host Redis服务器地址
     * @param port Redis服务器端口
     * @param password Redis密码（可选）
     * @param database 数据库索引（0-15）
     */
    RedisConnection(const std::string& host, int port,
                   const std::string& password, int database);

    /**
     * @brief 析构函数，自动释放连接
     */
    ~RedisConnection();

    /**
     * @brief 连接到Redis服务器
     * @return 成功返回true，失败返回false
     */
    bool connect();

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 检查连接状态
     * @return 已连接返回true，否则返回false
     */
    bool isConnected() const;

    // ========================================================================
    // 基本Redis操作
    // ========================================================================

    /**
     * @brief 设置键值
     * @param key 键名
     * @param value 值
     * @param ttl 过期时间（秒），0表示永不过期
     * @return 成功返回true，失败返回false
     */
    bool set(const std::string& key, const std::string& value, int ttl = 0);

    /**
     * @brief 获取键值
     * @param key 键名
     * @return 成功返回值，失败或不存在返回nullopt
     */
    std::optional<std::string> get(const std::string& key);

    /**
     * @brief 删除键
     * @param key 键名
     * @return 成功返回true，失败返回false
     */
    bool del(const std::string& key);

    /**
     * @brief 检查键是否存在
     * @param key 键名
     * @return 存在返回true，否则返回false
     */
    bool exists(const std::string& key);

    /**
     * @brief 设置键的过期时间
     * @param key 键名
     * @param ttl 过期时间（秒）
     * @return 成功返回true，失败返回false
     */
    bool expire(const std::string& key, int ttl);

    /**
     * @brief 获取键的剩余生存时间
     * @param key 键名
     * @return 剩余秒数，-1表示永不过期，-2表示键不存在
     */
    int ttl(const std::string& key);

    // ========================================================================
    // 批量操作
    // ========================================================================

    /**
     * @brief 批量设置键值
     * @param kvs 键值对map
     * @return 成功返回true，失败返回false
     */
    bool mset(const std::map<std::string, std::string>& kvs);

    /**
     * @brief 批量获取键值
     * @param keys 键名列表
     * @return 键值对map（不存在的键不会出现在结果中）
     */
    std::map<std::string, std::string> mget(const std::vector<std::string>& keys);

    // ========================================================================
    // 高级操作
    // ========================================================================

    /**
     * @brief 递增键值
     * @param key 键名
     * @param delta 递增量（默认1）
     * @return 递增后的值
     */
    int64_t incr(const std::string& key, int64_t delta = 1);

    /**
     * @brief 查找匹配的键
     * @param pattern 匹配模式（如 "user:*", "paper:*"）
     * @return 匹配的键名列表
     */
    std::vector<std::string> keys(const std::string& pattern);

    /**
     * @brief 清空当前数据库的所有键
     * @return 成功返回true，失败返回false
     */
    bool flushAll();

    /**
     * @brief Ping Redis服务器
     * @return 成功返回"PONG"，失败返回空字符串
     */
    std::string ping();

    // ========================================================================
    // Pipeline支持
    // ========================================================================

    /**
     * @brief 开始Pipeline模式（批量命令）
     *
     * 在pipelineBegin()之后的所有命令都会被缓冲，
     * 直到调用pipelineExecute()才会一次性发送到服务器
     */
    void pipelineBegin();

    /**
     * @brief 执行Pipeline中的所有命令
     * @return 成功返回true，失败返回false
     */
    bool pipelineExecute();

private:
#ifdef USE_REDIS_CACHE
    std::unique_ptr<redisContext, decltype(&redisFree)> context_;
#endif

    std::string host_;
    int port_;
    std::string password_;
    int database_;
    bool connected_;
    bool inPipeline_;

    /**
     * @brief 执行Redis命令（内部方法）
     * @param command Redis命令
     * @return Redis回复对象
     */
#ifdef USE_REDIS_CACHE
    redisReply* executeCommand(const char* format, ...);
#endif
};

} // namespace PaperCrawler
