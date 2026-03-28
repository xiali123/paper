#include "modules/system/QueueModule.hpp"
#include "handler/ResponseHandlerModule.hpp"
#include <sstream>
#include <iomanip>
#include <random>

namespace PaperCrawler {

// ============================================================================
// QueueStats JSON 序列化
// ============================================================================

std::string QueueStats::toJSON() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"current_size\": " << currentSize << ",\n";
    json << "  \"max_size\": " << maxSize << ",\n";
    json << "  \"total_enqueued\": " << totalEnqueued << ",\n";
    json << "  \"total_dequeued\": " << totalDequeued << ",\n";
    json << "  \"total_rejected\": " << totalRejected << ",\n";
    json << "  \"total_timeout\": " << totalTimeout << ",\n";
    json << "  \"average_wait_time_ms\": " << averageWaitTime << ",\n";

    // 按优先级统计
    json << "  \"count_by_priority\": {\n";
    bool first = true;
    for (const auto& pair : countByPriority) {
        if (!first) json << ",\n";
        first = false;

        json << "    \"priority_" << pair.first << "\": " << pair.second;
    }
    json << "\n  }\n";

    json << "}";
    return json.str();
}

// ============================================================================
// QueueModule 实现
// ============================================================================

class QueueModule::Impl {
public:
    std::priority_queue<QueueItem> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    size_t maxSize_;
    std::atomic<bool> stopped_{false};

    // 统计
    QueueStats stats_;

    Impl(size_t maxSize) : maxSize_(maxSize) {
        stats_.maxSize = maxSize;
    }

    /**
     * @brief 入队
     */
    bool enqueue(const QueueItem& item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.size() >= maxSize_) {
            stats_.totalRejected++;
            std::cout << "[Queue] Queue full, rejected request: " << item.requestId << std::endl;
            return false;
        }

        queue_.push(item);
        stats_.totalEnqueued++;
        stats_.currentSize = queue_.size();
        stats_.countByPriority[item.priority]++;

        std::cout << "[Queue] Enqueued: " << item.requestId
                  << " (priority: " << item.priority << ", size: " << queue_.size() << ")" << std::endl;

        condition_.notify_one();

        return true;
    }

    /**
     * @brief 出队（阻塞）
     */
    QueueItem dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);

        // 等待直到有项可用
        condition_.wait(lock, [this] {
            return !queue_.empty() || stopped_.load();
        });

        if (stopped_.load() && queue_.empty()) {
            throw std::runtime_error("Queue stopped");
        }

        QueueItem item = queue_.top();
        queue_.pop();

        auto now = std::chrono::system_clock::now();
        auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - item.queuedAt);

        stats_.totalDequeued++;
        stats_.currentSize = queue_.size();

        // 更新平均等待时间
        stats_.averageWaitTime = (stats_.averageWaitTime * (stats_.totalDequeued - 1) + waitTime.count()) / stats_.totalDequeued;

        std::cout << "[Queue] Dequeued: " << item.requestId
                  << " (waited: " << waitTime.count() << "ms, size: " << queue_.size() << ")" << std::endl;

        return item;
    }

    /**
     * @brief 尝试出队（非阻塞）
     */
    bool tryDequeue(QueueItem& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!condition_.wait_for(lock, timeout, [this] {
            return !queue_.empty() || stopped_.load();
        })) {
            stats_.totalTimeout++;
            return false;
        }

        if (stopped_.load() && queue_.empty()) {
            return false;
        }

        item = queue_.top();
        queue_.pop();

        auto now = std::chrono::system_clock::now();
        auto waitTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - item.queuedAt);

        stats_.totalDequeued++;
        stats_.currentSize = queue_.size();

        // 更新平均等待时间
        stats_.averageWaitTime = (stats_.averageWaitTime * (stats_.totalDequeued - 1) + waitTime.count()) / stats_.totalDequeued;

        return true;
    }

    /**
     * @brief 获取队列大小
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief 检查是否为空
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief 检查是否已满
     */
    bool full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size() >= maxSize_;
    }

    /**
     * @brief 获取统计
     */
    QueueStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        QueueStats stats = stats_;
        stats.currentSize = queue_.size();
        stats.countByPriority.clear();

        // 重新计算按优先级统计
        std::priority_queue<QueueItem> tempQueue = queue_;
        while (!tempQueue.empty()) {
            auto item = tempQueue.top();
            tempQueue.pop();
            stats.countByPriority[item.priority]++;
        }

        return stats;
    }

    /**
     * @brief 设置最大大小
     */
    void setMaxSize(size_t maxSize) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxSize_ = maxSize;
        stats_.maxSize = maxSize;
    }

    /**
     * @brief 清空队列
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);

        while (!queue_.empty()) {
            queue_.pop();
        }

        stats_.currentSize = 0;
        std::cout << "[Queue] Queue cleared" << std::endl;
    }

    /**
     * @brief 停止队列
     */
    void stop() {
        stopped_.store(true);
        condition_.notify_all();
    }
};

// ============================================================================

QueueModule::QueueModule(size_t maxSize)
    : impl_(std::make_unique<Impl>(maxSize)) {
}

QueueModule::~QueueModule() = default;

std::string QueueModule::getName() const {
    return "Queue";
}

std::string QueueModule::getVersion() const {
    return "1.0.0";
}

std::string QueueModule::getDescription() const {
    return "Request queue with priority support";
}

ModuleType QueueModule::getModuleType() const {
    return ModuleType::SERVER;
}

std::string QueueModule::getRoutePrefix() const {
    return "/api/queue";
}

bool QueueModule::initialize() {
    std::cout << "QueueModule::initialize (max_size: " << impl_->maxSize_ << ")" << std::endl;
    return true;
}

bool QueueModule::start() {
    std::cout << "QueueModule started" << std::endl;
    return true;
}

bool QueueModule::stop() {
    std::cout << "QueueModule stopped" << std::endl;
    impl_->stop();
    return true;
}

void QueueModule::cleanup() {
    impl_->clear();
}

bool QueueModule::enqueue(const QueueItem& item) {
    return impl_->enqueue(item);
}

size_t QueueModule::enqueueBatch(const std::vector<QueueItem>& items) {
    size_t enqueued = 0;

    for (const auto& item : items) {
        if (impl_->enqueue(item)) {
            enqueued++;
        }
    }

    std::cout << "[Queue] Batch enqueued: " << enqueued << "/" << items.size() << std::endl;

    return enqueued;
}

QueueItem QueueModule::dequeue() {
    return impl_->dequeue();
}

bool QueueModule::tryDequeue(QueueItem& item, std::chrono::milliseconds timeout) {
    return impl_->tryDequeue(item, timeout);
}

size_t QueueModule::size() const {
    return impl_->size();
}

bool QueueModule::empty() const {
    return impl_->empty();
}

bool QueueModule::full() const {
    return impl_->full();
}

QueueStats QueueModule::getStats() const {
    return impl_->getStats();
}

void QueueModule::setMaxSize(size_t maxSize) {
    impl_->setMaxSize(maxSize);
}

void QueueModule::clear() {
    impl_->clear();
}

// ============================================================================
// 路由处理
// ============================================================================

void QueueModule::registerRoutes() {
    // TODO: 注册路由到Router
}

std::string QueueModule::handleEnqueue(const std::string& body) {
    // TODO: 解析JSON body
    QueueItem item;
    item.requestId = "req_" + std::to_string(std::time(nullptr));
    item.method = "GET";
    item.path = "/api/papers";
    item.priority = static_cast<int>(RequestPriority::MEDIUM);
    item.queuedAt = std::chrono::system_clock::now();

    if (enqueue(item)) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"message", "Request enqueued"},
            {"request_id", item.requestId}
        });
    } else {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Queue full"}
        }, 503);
    }
}

std::string QueueModule::handleDequeue() {
    try {
        auto item = dequeue();

        return ResponseHandlerModule::buildJsonResponse({
            {"success", "true"},
            {"request_id", item.requestId},
            {"method", item.method},
            {"path", item.path}
        });
    } catch (const std::exception& e) {
        return ResponseHandlerModule::buildJsonResponse({
            {"success", "false"},
            {"error", "Queue empty or stopped"}
        }, 503);
    }
}

std::string QueueModule::handleStats() {
    auto stats = getStats();

    return ResponseHandlerModule::buildJsonResponse({
        {"stats", stats.toJSON()}
    });
}

} // namespace PaperCrawler
