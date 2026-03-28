#include "core/MessagePool.hpp"
#include <iostream>
#include <algorithm>

namespace PaperCrawler {

MessagePool& MessagePool::getInstance() {
    static MessagePool instance;
    return instance;
}

MessagePool::~MessagePool() {
    shutdown();
}

bool MessagePool::initialize(const MessagePoolConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        std::cerr << "MessagePool already initialized" << std::endl;
        return false;
    }

    config_ = config;

    // 预分配消息对象
    messages_.reserve(config_.poolSize);
    for (size_t i = 0; i < config_.poolSize; ++i) {
        auto msg = allocateMessage();
        if (msg) {
            messages_.push_back(msg);
        }
    }

    initialized_ = true;
    std::cout << "MessagePool initialized with " << messages_.size() << " messages" << std::endl;
    return true;
}

void MessagePool::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return;
    }

    // 清理所有消息
    messages_.clear();
    initialized_ = false;

    std::cout << "MessagePool shut down" << std::endl;
}

std::shared_ptr<PooledMessage> MessagePool::acquireMessage(int currentThreadId) {
    std::unique_lock<std::mutex> lock(mutex_);

    // 查找空闲的消息对象
    for (auto& msg : messages_) {
        if (!msg->inUse.load()) {
            msg->inUse = true;
            msg->lastUsed = std::chrono::system_clock::now();

            if (currentThreadId >= 0) {
                msg->threadAffinity = currentThreadId;
            }

            totalAllocations_++;
            return msg;
        }
    }

    // 池已满，尝试扩展
    if (messages_.size() < config_.maxPoolSize) {
        auto msg = allocateMessage();
        if (msg) {
            msg->inUse = true;
            msg->lastUsed = std::chrono::system_clock::now();

            if (currentThreadId >= 0) {
                msg->threadAffinity = currentThreadId;
            }

            messages_.push_back(msg);
            totalAllocations_++;
            return msg;
        }
    }

    // 等待消息对象可用
    condition_.wait(lock, [this]() {
        for (const auto& msg : messages_) {
            if (!msg->inUse.load()) {
                return true;
            }
        }
        return false;
    });

    // 再次尝试获取
    for (auto& msg : messages_) {
        if (!msg->inUse.load()) {
            msg->inUse = true;
            msg->lastUsed = std::chrono::system_clock::now();

            if (currentThreadId >= 0) {
                msg->threadAffinity = currentThreadId;
            }

            totalAllocations_++;
            return msg;
        }
    }

    return nullptr;
}

void MessagePool::releaseMessage(std::shared_ptr<PooledMessage> message) {
    if (!message) {
        return;
    }

    message->reset();
    totalReleases_++;
    condition_.notify_one();
}

std::shared_ptr<PooledMessage> MessagePool::createFromUnified(
    const UnifiedMessage& msg,
    int currentThreadId
) {
    auto pooledMsg = acquireMessage(currentThreadId);

    if (pooledMsg) {
        // 分配缓冲区（如果需要）
        if (!pooledMsg->buffer) {
            pooledMsg->bufferSize = config_.messageBufferSize;
            pooledMsg->buffer = std::malloc(pooledMsg->bufferSize);
        }

        // 关联统一消息
        pooledMsg->unifiedMessage = const_cast<UnifiedMessage*>(&msg);
    }

    return pooledMsg;
}

UnifiedMessage MessagePool::toUnifiedMessage(const std::shared_ptr<PooledMessage>& pooled) {
    if (pooled && pooled->unifiedMessage) {
        return *pooled->unifiedMessage;
    }

    // 返回空消息
    return UnifiedMessage();
}

int MessagePool::selectWorkerThread(const std::shared_ptr<PooledMessage>& message) {
    std::lock_guard<std::mutex> lock(threadTaskMutex_);

    if (!message || threadTaskCount_.empty()) {
        return 0;
    }

    int strategy = config_.loadBalanceStrategy;

    switch (strategy) {
        case 0: { // 轮询
            size_t index = roundRobinIndex_.fetch_add(1) % threadTaskCount_.size();
            return index;
        }

        case 1: { // 最少任务
            auto it = std::min_element(threadTaskCount_.begin(), threadTaskCount_.end(),
                [](const auto& a, const auto& b) {
                    return a.second < b.second;
                });
            return it->first;
        }

        case 2: { // 随机
            size_t index = rand() % threadTaskCount_.size();
            return index;
        }

        default:
            return 0;
    }
}

MessagePool::PoolStats MessagePool::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    PoolStats stats{};
    stats.totalMessages = messages_.size();
    stats.availableMessages = 0;
    stats.activeMessages = 0;
    stats.totalAllocations = totalAllocations_.load();
    stats.totalReleases = totalReleases_.load();

    uint64_t totalTime = 0;
    uint64_t totalUseCount = 0;

    for (const auto& msg : messages_) {
        if (!msg->inUse.load()) {
            stats.availableMessages++;
        } else {
            stats.activeMessages++;
        }

        totalTime += msg->totalProcessingTime.count();
        totalUseCount += msg->useCount;

        // 统计每线程的消息数
        if (msg->threadAffinity >= 0) {
            stats.messagesPerThread[msg->threadAffinity]++;
        }
    }

    if (totalUseCount > 0) {
        stats.averageProcessingTime = static_cast<double>(totalTime) / totalUseCount;
    }

    // 计算内存复用率
    if (stats.totalAllocations > 0) {
        stats.memoryReuseRate = 1.0 - static_cast<double>(messages_.size()) / stats.totalAllocations;
    }

    return stats;
}

void MessagePool::resize(size_t newSize) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (newSize > config_.maxPoolSize) {
        std::cerr << "Cannot resize pool beyond max size: " << config_.maxPoolSize << std::endl;
        return;
    }

    // 扩展或收缩
    if (newSize > messages_.size()) {
        size_t toAdd = newSize - messages_.size();
        for (size_t i = 0; i < toAdd; ++i) {
            auto msg = allocateMessage();
            if (msg) {
                messages_.push_back(msg);
            }
        }
    } else if (newSize < messages_.size()) {
        // 收缩（只移除空闲的）
        auto it = std::remove_if(messages_.begin(), messages_.end(),
            [](const auto& msg) {
                return !msg->inUse.load();
            });
        messages_.erase(it, messages_.end());
    }

    std::cout << "MessagePool resized to " << messages_.size() << std::endl;
}

void MessagePool::cleanupIdleMessages(std::chrono::seconds idleTimeout) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    size_t cleaned = 0;

    for (auto& msg : messages_) {
        if (!msg->inUse.load()) {
            auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(
                now - msg->lastUsed
            );

            if (idleTime > idleTimeout && messages_.size() > config_.poolSize) {
                // 释放缓冲区
                if (msg->buffer) {
                    std::free(msg->buffer);
                    msg->buffer = nullptr;
                }
                cleaned++;
            }
        }
    }

    if (cleaned > 0) {
        std::cout << "Cleaned up " << cleaned << " idle messages" << std::endl;
    }
}

std::shared_ptr<PooledMessage> MessagePool::allocateMessage() {
    auto msg = std::make_shared<PooledMessage>();
    msg->buffer = std::malloc(config_.messageBufferSize);
    msg->bufferSize = config_.messageBufferSize;
    msg->usedSize = 0;
    msg->inUse = false;
    msg->threadAffinity = -1;
    msg->lastUsed = std::chrono::system_clock::now();
    msg->useCount = 0;
    msg->totalProcessingTime = std::chrono::microseconds{0};
    msg->unifiedMessage = nullptr;

    return msg;
}

void MessagePool::updateStats(std::chrono::microseconds processingTime) {
    totalProcessingTimeUs_ += processingTime.count();
}

} // namespace PaperCrawler
