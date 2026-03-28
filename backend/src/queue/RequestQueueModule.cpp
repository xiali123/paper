#include "queue/RequestQueueModule.hpp"
#include <iostream>

namespace PaperCrawler {

RequestQueueModule::RequestQueueModule(size_t maxQueueSize)
    : maxQueueSize_(maxQueueSize) {}

RequestQueueModule::~RequestQueueModule() {
    stop();
}

bool RequestQueueModule::initialize() {
    std::cout << "RequestQueueModule initialized (max size: "
              << maxQueueSize_ << ")" << std::endl;
    return true;
}

bool RequestQueueModule::start() {
    stop_ = false;
    std::cout << "RequestQueueModule started" << std::endl;
    return true;
}

bool RequestQueueModule::stop() {
    stop_ = true;
    condition_.notify_all();
    std::cout << "RequestQueueModule stopped" << std::endl;
    return true;
}

void RequestQueueModule::cleanup() {
    stop();
}

bool RequestQueueModule::enqueue(const RequestQueueItem& item) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (queue_.size() >= maxQueueSize_) {
        totalRejected_++;
        return false;  // 队列已满
    }

    queue_.push(item);
    totalEnqueued_++;
    condition_.notify_one();
    return true;
}

size_t RequestQueueModule::enqueueBatch(const std::vector<RequestQueueItem>& items) {
    size_t enqueued = 0;
    for (const auto& item : items) {
        if (enqueue(item)) {
            enqueued++;
        }
    }
    return enqueued;
}

RequestQueueItem RequestQueueModule::dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);

    condition_.wait(lock, [this]() {
        return !queue_.empty() || stop_;
    });

    if (queue_.empty()) {
        RequestQueueItem item;
        item.requestId = "empty";
        return item;
    }

    RequestQueueItem item = queue_.top();
    queue_.pop();
    totalDequeued_++;
    return item;
}

bool RequestQueueModule::tryDequeue(RequestQueueItem& item, std::chrono::milliseconds timeout) {
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

RequestQueueModule::QueueStats RequestQueueModule::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    QueueStats stats{};
    stats.currentSize = queue_.size();
    stats.maxSize = maxQueueSize_;
    stats.totalEnqueued = totalEnqueued_.load();
    stats.totalDequeued = totalDequeued_.load();
    stats.totalRejected = totalRejected_.load();
    stats.averageWaitTime = 0.0;

    // 统计各优先级分布
    std::priority_queue<RequestQueueItem, std::vector<RequestQueueItem>, std::greater<RequestQueueItem>> tempQueue = queue_;
    while (!tempQueue.empty()) {
        auto priority = tempQueue.top().priority;
        stats.priorityDistribution[priority]++;
        tempQueue.pop();
    }

    return stats;
}

RequestPriority RequestQueueModule::assignPriority(const HttpRequest& request) {
    // TODO: 根据请求特征分配优先级
    return RequestPriority::MEDIUM;
}

} // namespace PaperCrawler
