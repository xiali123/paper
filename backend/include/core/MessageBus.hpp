#pragma once

#include "ModuleMessage.hpp"
#include "IModule.hpp"
#include <map>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>

namespace PaperCrawler {

/**
 * @brief 消息处理器类型
 */
using MessageHandler = std::function<std::shared_ptr<ModuleMessage>(std::shared_ptr<ModuleMessage>)>;

/**
 * @brief 消息总线
 *
 * 负责模块间的消息传递
 */
class MessageBus {
public:
    /**
     * @brief 获取单例
     */
    static MessageBus& getInstance();

    /**
     * @brief 注册消息处理器
     * @param messageType 消息类型
     * @param handler 处理器函数
     * @param moduleName 模块名称
     */
    void registerHandler(MessageType messageType,
                        MessageHandler handler,
                        const std::string& moduleName);

    /**
     * @brief 注销消息处理器
     */
    void unregisterHandler(MessageType messageType, const std::string& moduleName);

    /**
     * @brief 发送消息（异步）
     */
    void send(std::shared_ptr<ModuleMessage> message);

    /**
     * @brief 同步发送消息（等待结果）
     */
    std::shared_ptr<ModuleMessage> sendSync(
        std::shared_ptr<ModuleMessage> message,
        uint32_t timeoutMs = 0
    );

    /**
     * @brief 广播消息到所有处理器
     */
    void broadcast(std::shared_ptr<ModuleMessage> message);

    /**
     * @brief 获取统计信息
     */
    struct Stats {
        uint64_t totalMessages;
        uint64_t totalHandlers;
        std::map<MessageType, size_t> handlersPerType;
    };
    Stats getStats() const;

private:
    MessageBus() = default;
    ~MessageBus() = default;

    // 禁止拷贝
    MessageBus(const MessageBus&) = delete;
    MessageBus& operator=(const MessageBus&) = delete;

    std::map<MessageType, std::map<std::string, MessageHandler>> handlers_;
    mutable std::mutex mutex_;

    // 统计
    std::atomic<uint64_t> totalMessages_{0};
};

} // namespace PaperCrawler
