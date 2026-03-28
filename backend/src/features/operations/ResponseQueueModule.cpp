#include "features/infrastructure/ResponseQueueModule.hpp"
#include <iostream>

namespace PaperCrawler {

ResponseQueueModule::ResponseQueueModule(size_t maxQueueSize)
    : maxQueueSize_(maxQueueSize) {}

ResponseQueueModule::~ResponseQueueModule() {
    stop();
}

bool ResponseQueueModule::initialize() {
    std::cout << "ResponseQueueModule initialized (max size: "
              << maxQueueSize_ << ")" << std::endl;
    return true;
}

bool ResponseQueueModule::start() {
    stop_ = false;
    std::cout << "ResponseQueueModule started" << std::endl;
    return true;
}

bool ResponseQueueModule::stop() {
    stop_ = true;
    condition_.notify_all();
    std::cout << "ResponseQueueModule stopped" << std::endl;
    return true;
}

void ResponseQueueModule::cleanup() {
    stop();
}

bool ResponseQueueModule::enqueue(const ResponseQueueItem& item) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (queue_.size() >= maxQueueSize_) {
        return false;  // 队列已满
    }

    queue_.push(item);
    totalEnqueued_++;
    condition_.notify_one();
    return true;
}

size_t ResponseQueueModule::enqueueBatch(const std::vector<ResponseQueueItem>& items) {
    size_t enqueued = 0;
    for (const auto& item : items) {
        if (enqueue(item)) {
            enqueued++;
        }
    }
    return enqueued;
}

ResponseQueueItem ResponseQueueModule::dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);

    condition_.wait(lock, [this]() {
        return !queue_.empty() || stop_;
    });

    if (queue_.empty()) {
        ResponseQueueItem item;
        item.messageId = "empty";
        return item;
    }

    ResponseQueueItem item = queue_.top();
    queue_.pop();
    totalDequeued_++;
    return item;
}

bool ResponseQueueModule::tryDequeue(ResponseQueueItem& item, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (condition_.wait_for(lock, timeout, [this]() {
        return !queue_.empty() || stop_;
    })) {
        if (queue_.empty()) {
            return false;
        }

        item = queue_.top();
        queue_.pop();
        totalDequeued_++;
        return true;
    }

    return false;
}

ResponseQueueModule::QueueStats ResponseQueueModule::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    QueueStats stats{};
    stats.currentSize = queue_.size();
    stats.maxSize = maxQueueSize_;
    stats.totalEnqueued = totalEnqueued_.load();
    stats.totalDequeued = totalDequeued_.load();
    stats.totalTimeout = 0;
    stats.averageWaitTime = 0.0;

    return stats;
}

} // namespace PaperCrawler
