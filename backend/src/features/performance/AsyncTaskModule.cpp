#include "features/performance/AsyncTaskModule.hpp"
#include <iostream>
#include <sstream>
#include <atomic>
#include <thread>
#include <queue>

namespace PaperCrawler {

// ============================================================================
// AsyncTaskModule::Impl
// ============================================================================

class AsyncTaskModule::Impl {
public:
    std::map<std::string, AsyncTask> tasks_;
    std::map<int, std::deque<std::string>> priorityQueues_;  // 优先级队列
    std::atomic<uint64_t> taskIdCounter_{0};
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;

    Impl() {
        // 初始化优先级队列
        priorityQueues_[-1] = std::deque<std::string>();  // 低优先级
        priorityQueues_[0] = std::deque<std::string>();   // 普通优先级
        priorityQueues_[1] = std::deque<std::string>();   // 高优先级
    }

    std::string submit(const std::string& name, std::function<void()> task, int priority) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 生成任务ID
        std::ostringstream oss;
        oss << "task_" << std::time(nullptr) << "_" << taskIdCounter_.fetch_add(1);
        std::string taskId = oss.str();

        // 创建任务
        AsyncTask asyncTask;
        asyncTask.taskId = taskId;
        asyncTask.name = name;
        asyncTask.task = task;
        asyncTask.status = AsyncTaskStatus::PENDING;
        asyncTask.createdAt = std::chrono::system_clock::now();

        tasks_[taskId] = asyncTask;

        // 加入优先级队列
        priorityQueues_[priority].push_back(taskId);

        std::cout << "[AsyncTask] Submitted: " << taskId
                  << " (priority: " << priority << ")" << std::endl;

        return taskId;
    }

    bool cancel(const std::string& taskId) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = tasks_.find(taskId);
        if (it == tasks_.end()) {
            return false;
        }

        if (it->second.status == AsyncTaskStatus::RUNNING) {
            std::cout << "[AsyncTask] Cannot cancel running task: " << taskId << std::endl;
            return false;
        }

        it->second.status = AsyncTaskStatus::CANCELLED;

        // 从优先级队列中移除
        for (auto& [priority, queue] : priorityQueues_) {
            auto qIt = std::find(queue.begin(), queue.end(), taskId);
            if (qIt != queue.end()) {
                queue.erase(qIt);
                break;
            }
        }

        std::cout << "[AsyncTask] Cancelled: " << taskId << std::endl;
        return true;
    }

    bool executeNext() {
        std::lock_guard<std::mutex> lock(mutex_);

        // 按优先级查找任务：高 → 普通 → 低
        std::vector<int> priorities = {1, 0, -1};

        for (int priority : priorities) {
            auto& queue = priorityQueues_[priority];
            if (!queue.empty()) {
                std::string taskId = queue.front();
                queue.pop_front();

                auto it = tasks_.find(taskId);
                if (it != tasks_.end() && it->second.status == AsyncTaskStatus::PENDING) {
                    // 执行任务
                    executeTask(it->second);
                    return true;
                }
            }
        }

        return false;
    }

private:
    void executeTask(AsyncTask& task) {
        task.status = AsyncTaskStatus::RUNNING;
        task.startedAt = std::chrono::system_clock::now();

        std::cout << "[AsyncTask] Executing: " << task.taskId << std::endl;

        try {
            if (task.task) {
                task.task();
            }
            task.status = AsyncTaskStatus::COMPLETED;
            task.completedAt = std::chrono::system_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                task.completedAt - task.startedAt);

            std::cout << "[AsyncTask] Completed: " << task.taskId
                      << " (duration: " << duration.count() << "ms)" << std::endl;

        } catch (const std::exception& e) {
            task.status = AsyncTaskStatus::FAILED;
            task.errorMessage = e.what();
            task.completedAt = std::chrono::system_clock::now();

            std::cout << "[AsyncTask] Failed: " << task.taskId
                      << " (error: " << e.what() << ")" << std::endl;
        }
    }
};

// ============================================================================
// AsyncTaskModule
// ============================================================================

AsyncTaskModule::AsyncTaskModule()
    : impl_(std::make_unique<Impl>()) {}

AsyncTaskModule::~AsyncTaskModule() = default;

bool AsyncTaskModule::initialize() {
    std::cout << "AsyncTaskModule::initialize" << std::endl;
    return true;
}

bool AsyncTaskModule::start() {
    std::cout << "AsyncTaskModule started" << std::endl;
    impl_->running_ = true;
    return true;
}

bool AsyncTaskModule::stop() {
    std::cout << "AsyncTaskModule stopped" << std::endl;
    impl_->running_ = false;
    return true;
}

void AsyncTaskModule::cleanup() {
    // 清理所有任务
}

std::string AsyncTaskModule::submit(const std::string& name,
                                   std::function<void()> task,
                                   int priority) {
    return impl_->submit(name, task, priority);
}

bool AsyncTaskModule::cancel(const std::string& taskId) {
    return impl_->cancel(taskId);
}

} // namespace PaperCrawler
