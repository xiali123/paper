#include "core/MessagePool.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace PaperCrawler {

// 静态成员初始化
std::unique_ptr<MessagePool> MessagePool::instance_;
std::mutex MessagePool::instanceMutex_;

MessagePool& MessagePool::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex_);

    if (!instance_) {
        instance_ = std::unique_ptr<MessagePool>(new MessagePool());
    }

    return *instance_;
}

MessagePool::~MessagePool() {
    shutdown();
}

bool MessagePool::initialize(const MessagePoolConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    spdlog::info("Initializing MessagePool with {} messages", config.poolSize);

    config_ = config;

    // 预分配消息对象
    messages_.reserve(config_.poolSize);
    for (size_t i = 0; i < config_.poolSize; ++i) {
        auto msg = std::make_shared<PooledMessage>();
        msg->bufferSize = config_.messageBufferSize;
        // 使用智能指针分配buffer（RAII自动管理）
        msg->buffer = std::make_unique<uint8_t[]>(msg->bufferSize);
        msg->inUse = false;
        msg->useCount = 0;
        msg->threadAffinity = -1;
        msg->lastUsed = std::chrono::system_clock::now();

        messages_.push_back(msg);
    }

    spdlog::info("MessagePool initialized with {} messages", messages_.size());
    return true;
}

void MessagePool::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    spdlog::info("Shutting down MessagePool");

    // 智能指针自动管理内存，无需手动delete
    // reset unique_ptr会自动释放内存
    for (auto& msg : messages_) {
        if (msg->buffer) {
            msg->buffer.reset(); // unique_ptr自动释放内存
            msg->bufferSize = 0;
        }
    }
    messages_.clear();
}

std::shared_ptr<PooledMessage> MessagePool::acquireMessage(int currentThreadId) {
    std::unique_lock<std::mutex> lock(mutex_);

    // 查找空闲的消息对象（优先选择具有线程亲和性的）
    std::shared_ptr<PooledMessage> selectedMsg;

    if (currentThreadId >= 0) {
        // 首先尝试查找有相同线程亲和性的消息
        for (auto& msg : messages_) {
            if (!msg->inUse.load() && msg->threadAffinity == currentThreadId) {
                selectedMsg = msg;
                break;
            }
        }
    }

    // 如果没有找到亲和的消息，查找任意空闲消息
    if (!selectedMsg) {
        for (auto& msg : messages_) {
            if (!msg->inUse.load()) {
                selectedMsg = msg;
                break;
            }
        }
    }

    if (selectedMsg) {
        selectedMsg->inUse = true;
        selectedMsg->lastUsed = std::chrono::system_clock::now();

        if (currentThreadId >= 0) {
            selectedMsg->threadAffinity = currentThreadId;
            threadTaskCount_[currentThreadId]++;
        }

        totalAllocations_++;
        return selectedMsg;
    }

    // 池已满，尝试扩展
    if (messages_.size() < config_.maxPoolSize) {
        auto msg = std::make_shared<PooledMessage>();
        msg->bufferSize = config_.messageBufferSize;
        // 使用智能指针分配buffer（RAII自动管理）
        msg->buffer = std::make_unique<uint8_t[]>(msg->bufferSize);
        msg->inUse = true;
        msg->useCount = 0;
        msg->threadAffinity = currentThreadId >= 0 ? currentThreadId : -1;
        msg->lastUsed = std::chrono::system_clock::now();

        messages_.push_back(msg);
        totalAllocations_++;

        if (currentThreadId >= 0) {
            threadTaskCount_[currentThreadId]++;
        }

        spdlog::debug("Expanded message pool to {}", messages_.size());
        return msg;
    }

    // 等待消息对象可用（带超时）
    condition_.wait_for(lock, std::chrono::milliseconds(100), [this]() {
        return std::any_of(messages_.begin(), messages_.end(),
            [](const auto& msg) { return !msg->inUse.load(); });
    });

    // 再次尝试获取
    for (auto& msg : messages_) {
        if (!msg->inUse.load()) {
            msg->inUse = true;
            msg->lastUsed = std::chrono::system_clock::now();

            if (currentThreadId >= 0) {
                msg->threadAffinity = currentThreadId;
                threadTaskCount_[currentThreadId]++;
            }

            totalAllocations_++;
            return msg;
        }
    }

    spdlog::warn("Failed to acquire message from pool");
    return nullptr;
}

void MessagePool::releaseMessage(std::shared_ptr<PooledMessage> message) {
    if (!message) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    message->inUse = false;
    totalReleases_++;

    if (message->threadAffinity >= 0) {
        auto it = threadTaskCount_.find(message->threadAffinity);
        if (it != threadTaskCount_.end() && it->second > 0) {
            it->second--;
        }
    }

    // 通知等待的线程
    condition_.notify_one();
}

MessagePool::PoolStats MessagePool::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    PoolStats stats;
    stats.totalMessages = messages_.size();
    stats.availableMessages = 0;
    stats.activeMessages = 0;
    stats.waitingThreads = 0;
    stats.totalAllocations = totalAllocations_.load();
    stats.totalReleases = totalReleases_.load();

    uint64_t totalTime = totalProcessingTimeUs_.load();
    uint64_t totalCount = totalAllocations_.load();
    stats.averageProcessingTime = totalCount > 0 ?
        static_cast<double>(totalTime) / totalCount : 0.0;

    stats.messagesPerThread = threadTaskCount_;

    // 统计可用和活跃消息
    for (const auto& msg : messages_) {
        if (msg->inUse.load()) {
            stats.activeMessages++;
        } else {
            stats.availableMessages++;
        }
    }

    return stats;
}

} // namespace PaperCrawler
