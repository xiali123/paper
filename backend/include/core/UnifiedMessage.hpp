#pragma once

#include <string>
#include <map>
#include <any>
#include <chrono>
#include <functional>
#include <vector>
#include <mutex>
#include <atomic>

namespace PaperCrawler {

/**
 * @brief 统一的消息操作类型
 */
enum class MessageOperation {
    // CRUD操作
    CREATE = 0x0100,
    READ = 0x0101,
    UPDATE = 0x0102,
    DELETE = 0x0103,
    QUERY = 0x0104,

    // 批量操作
    BATCH_CREATE = 0x0200,
    BATCH_READ = 0x0201,
    BATCH_UPDATE = 0x0202,
    BATCH_DELETE = 0x0203,

    // 事务操作
    TRANSACTION_BEGIN = 0x0300,
    TRANSACTION_COMMIT = 0x0301,
    TRANSACTION_ROLLBACK = 0x0302,

    // 通知事件
    NOTIFY = 0x0400,
    SUBSCRIBE = 0x0401,
    UNSUBSCRIBE = 0x0402,

    // 系统操作
    HEALTH_CHECK = 0x0500,
    STATUS = 0x0501,
    PING = 0x0502,
    PONG = 0x0503,

    // 通用消息
    CUSTOM = 0xFFFF
};

/**
 * @brief 消息目标对象类型
 */
enum class MessageTarget {
    DATABASE,      // 数据库操作
    CACHE,         // 缓存操作
    FILE,          // 文件操作
    MODULE,        // 模块间调用
    SYSTEM,        // 系统级操作
    BROADCAST      // 广播
};

/**
 * @brief 统一的消息结构
 */
struct UnifiedMessage {
    // 消息标识
    std::string messageId;           // 唯一ID
    MessageOperation operation;      // 操作类型
    MessageTarget target;            // 目标类型
    std::string targetName;          // 目标名称（如：表名、缓存key、模块名）

    // 数据载荷
    std::map<std::string, std::any> parameters;  // 参数键值对
    std::any payload;                              // 主要数据负载

    // 源和目标
    std::string sourceModule;      // 发送者模块
    std::string targetModule;      // 接收者模块（空表示广播）

    // 元数据
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;
    int timeoutMs{5000};           // 超时时间
    bool requiresResponse{true};   // 是否需要响应

    // 优先级
    int priority{0};               // 0=普通, 1=高, -1=低

    /**
     * @brief 构造函数
     */
    UnifiedMessage(
        MessageOperation op,
        MessageTarget tgt,
        const std::string& tgtName
    ) : messageId(generateMessageId()),
        operation(op),
        target(tgt),
        targetName(tgtName),
        timestamp(std::chrono::system_clock::now()) {}

    /**
     * @brief 默认构造函数
     */
    UnifiedMessage()
        : messageId(generateMessageId()),
          operation(MessageOperation::CUSTOM),
          target(MessageTarget::SYSTEM),
          timestamp(std::chrono::system_clock::now()) {}

    /**
     * @brief 便捷方法：设置参数
     */
    template<typename T>
    void setParameter(const std::string& key, const T& value) {
        parameters[key] = value;
    }

    /**
     * @brief 便捷方法：获取参数
     */
    template<typename T>
    T getParameter(const std::string& key, const T& defaultValue = T{}) const {
        auto it = parameters.find(key);
        if (it != parameters.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (...) {
                return defaultValue;
            }
        }
        return defaultValue;
    }

    /**
     * @brief 创建响应消息
     */
    UnifiedMessage createResponse() const {
        UnifiedMessage response(
            operation == MessageOperation::PING ? MessageOperation::PONG : operation,
            target,
            targetName
        );
        response.messageId = messageId + "_resp";
        response.targetModule = sourceModule;
        response.sourceModule = targetModule;
        return response;
    }

    /**
     * @brief 转换为字符串（用于调试）
     */
    std::string toString() const;

private:
    static std::string generateMessageId() {
        static std::atomic<uint64_t> counter{0};
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        return "msg_" + std::to_string(timestamp) + "_" +
               std::to_string(counter.fetch_add(1));
    }
};

/**
 * @brief 消息响应包装
 */
struct MessageResponse {
    std::string messageId;
    bool success{false};
    std::string errorMessage;
    std::any data;
    std::map<std::string, std::any> metadata;
    std::chrono::milliseconds processingTime{0};
};

/**
 * @brief 统一消息总线（扩展MessageBus）
 */
class UnifiedMessageBus {
public:
    static UnifiedMessageBus& getInstance();

    /**
     * @brief 发送消息并等待响应
     */
    MessageResponse sendSync(const UnifiedMessage& message);

    /**
     * @brief 发送消息（异步）
     */
    void sendAsync(const UnifiedMessage& message,
                   std::function<void(const MessageResponse&)> callback);

    /**
     * @brief 广播消息
     */
    void broadcast(const UnifiedMessage& message);

    /**
     * @brief 注册消息处理器
     */
    using MessageHandler = std::function<MessageResponse(const UnifiedMessage&)>;
    void registerHandler(MessageTarget target,
                        const std::string& targetName,
                        MessageHandler handler);

    /**
     * @brief 订阅事件通知
     */
    void subscribe(const std::string& event,
                   std::function<void(const UnifiedMessage&)> handler);

    /**
     * @brief 取消订阅
     */
    void unsubscribe(const std::string& event, size_t handlerId);

private:
    UnifiedMessageBus() = default;
    ~UnifiedMessageBus() = default;

    std::map<std::pair<MessageTarget, std::string>, MessageHandler> handlers_;
    std::map<std::string, std::vector<std::pair<size_t, std::function<void(const UnifiedMessage&)>>>> subscribers_;
    std::atomic<size_t> nextSubscriberId_{0};
    std::mutex mutex_;
};

} // namespace PaperCrawler
