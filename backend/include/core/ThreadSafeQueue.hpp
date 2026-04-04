#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>
#include <functional>

namespace PaperCrawler {

/**
 * @brief 线程安全队列模板
 *
 * 特性：
 * 1. 完全线程安全（使用mutex和condition_variable）
 * 2. 支持阻塞和非阻塞操作
 * 3. 支持超时
 * 4. 支持移动语义（高性能）
 * 5. 支持批量操作
 * 6. 异常安全
 *
 * @tparam T 队列元素类型
 *
 * 使用示例：
 * @code
 * ThreadSafeQueue<Task> queue;
 *
 * // 生产者线程
 * queue.enqueue(Task{...});
 *
 * // 消费者线程（阻塞等待）
 * auto task = queue.dequeue();
 * if (task) {
 *     process(*task);
 * }
 *
 * // 消费者线程（带超时）
 * auto task = queue.dequeue(std::chrono::milliseconds(100));
 * @endcode
 */
template<typename T>
class ThreadSafeQueue {
public:
    using value_type = T;
    using size_type = typename std::queue<T>::size_type;

    /**
     * @brief 默认构造函数
     */
    ThreadSafeQueue() = default;

    /**
     * @brief 析构函数
     */
    ~ThreadSafeQueue() {
        shutdown();
    }

    // 禁止拷贝
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // 支持移动
    ThreadSafeQueue(ThreadSafeQueue&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        queue_ = std::move(other.queue_);
        shutdown_ = other.shutdown_;
        other.shutdown_ = true;
    }

    ThreadSafeQueue& operator=(ThreadSafeQueue&& other) noexcept {
        if (this != &other) {
            std::lock(mutex_, other.mutex_);
            std::lock_guard<std::mutex> lock1(mutex_, std::adopt_lock);
            std::lock_guard<std::mutex> lock2(other.mutex_, std::adopt_lock);
            queue_ = std::move(other.queue_);
            shutdown_ = other.shutdown_;
            other.shutdown_ = true;
        }
        return *this;
    }

    /**
     * @brief 入队（拷贝）
     * @param item 要入队的元素
     */
    void enqueue(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
        condition_.notify_one();
    }

    /**
     * @brief 入队（移动）
     * @param item 要入队的元素
     */
    void enqueue(T&& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        condition_.notify_one();
    }

    /**
     * @brief 批量入队
     * @param items 要入队的元素列表
     */
    void enqueueBatch(const std::vector<T>& items) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& item : items) {
            queue_.push(item);
        }
        condition_.notify_all();
    }

    /**
     * @brief 出队（阻塞等待）
     * @return 队列元素，如果队列为空且已关闭则返回std::nullopt
     */
    std::optional<T> dequeue() {
        std::unique_lock<std::mutex> lock(mutex_);

        // 等待队列非空或关闭
        condition_.wait(lock, [this] {
            return !queue_.empty() || shutdown_;
        });

        // 检查是否已关闭且队列为空
        if (queue_.empty() && shutdown_) {
            return std::nullopt;
        }

        // 获取元素
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /**
     * @brief 出队（带超时）
     * @param timeout 超时时间
     * @return 队列元素，如果超时或队列为空且已关闭则返回std::nullopt
     */
    template<typename Rep, typename Period>
    std::optional<T> dequeue(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mutex_);

        // 等待队列非空或关闭（带超时）
        if (!condition_.wait_for(lock, timeout, [this] {
            return !queue_.empty() || shutdown_;
        })) {
            // 超时
            return std::nullopt;
        }

        // 检查是否已关闭且队列为空
        if (queue_.empty() && shutdown_) {
            return std::nullopt;
        }

        // 获取元素
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /**
     * @brief 尝试出队（非阻塞）
     * @return 队列元素，如果队列为空则返回std::nullopt
     */
    std::optional<T> tryDequeue() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    /**
     * @brief 检查队列是否为空
     * @return true如果队列为空
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief 获取队列大小
     * @return 队列中的元素数量
     */
    size_type size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief 关闭队列
     *
     * 关闭后，所有阻塞的wait操作都会返回，
     * 并且无法再向队列中添加元素。
     */
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
        condition_.notify_all();
    }

    /**
     * @brief 检查队列是否已关闭
     * @return true如果队列已关闭
     */
    bool isShutdown() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shutdown_;
    }

    /**
     * @brief 清空队列
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
        condition_.notify_all();
    }

    /**
     * @brief 交换队列内容
     * @param other 要交换的队列
     */
    void swap(std::queue<T>& other) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::swap(queue_, other);
        condition_.notify_all();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<T> queue_;
    bool shutdown_{false};
};

} // namespace PaperCrawler
