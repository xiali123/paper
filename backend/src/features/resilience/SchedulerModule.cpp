#include "features/resilience/SchedulerModule.hpp"
#include <iostream>
#include <thread>
#include <algorithm>

namespace PaperCrawler {

// ============================================================================
// SchedulerModule 实现
// ============================================================================

class SchedulerModule::Impl {
public:
    std::map<JobId, ScheduledJob> jobs_;
    std::thread schedulerThread_;
    std::atomic<bool> running_{false};
    std::atomic<JobId> nextJobId_{1};
    mutable std::mutex mutex_;

    void schedulerLoop() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            std::lock_guard<std::mutex> lock(mutex_);

            auto now = std::chrono::system_clock::now();

            for (auto& pair : jobs_) {
                auto& job = pair.second;

                if (job.status == JobStatus::PENDING && now >= job.nextRunTime) {
                    std::cout << "[Scheduler] Executing job: " << job.name << std::endl;

                    // 标记为运行中
                    job.status = JobStatus::RUNNING;
                    job.lastRunAt = now;

                    // 在新线程中执行任务
                    std::thread([this, &job]() {
                        try {
                            auto startTime = std::chrono::high_resolution_clock::now();

                            if (job.task) {
                                job.task();
                            }

                            auto endTime = std::chrono::high_resolution_clock::now();
                            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

                            job.totalRuns++;
                            job.successRuns++;
                            job.lastSuccessAt = std::chrono::system_clock::now();
                            job.totalRunTime += duration;
                            job.status = JobStatus::COMPLETED;

                            std::cout << "[Scheduler] Job completed: " << job.name << " (" << duration.count() << "μs)" << std::endl;

                        } catch (const std::exception& e) {
                            job.totalRuns++;
                            job.failedRuns++;
                            job.lastFailureAt = std::chrono::system_clock::now();
                            job.status = JobStatus::FAILED;

                            std::cerr << "[Scheduler] Job failed: " << job.name << " - " << e.what() << std::endl;
                        }

                        // 计算下次运行时间
                        updateNextRunTime(job);
                    }).detach();
                }
            }
        }
    }

    void updateNextRunTime(ScheduledJob& job) {
        if (job.interval.count() > 0) {
            // 重复任务
            job.nextRunTime = std::chrono::system_clock::now() + job.interval;
            job.status = JobStatus::PENDING;
        } else if (!job.cronExpression.empty()) {
            // Cron任务（简化实现）
            job.nextRunTime = std::chrono::system_clock::now() + std::chrono::minutes(1);
            job.status = JobStatus::PENDING;
        } else {
            // 一次性任务
            job.status = JobStatus::COMPLETED;
        }
    }
};

SchedulerModule::SchedulerModule()
    : impl_(std::make_unique<Impl>()) {}

SchedulerModule::~SchedulerModule() {
    stop();
}

bool SchedulerModule::initialize() {
    std::cout << "SchedulerModule::initialize" << std::endl;
    return true;
}

bool SchedulerModule::start() {
    std::cout << "SchedulerModule started" << std::endl;

    impl_->running_ = true;
    impl_->schedulerThread_ = std::thread(&SchedulerModule::Impl::schedulerLoop, impl_.get());

    return true;
}

bool SchedulerModule::stop() {
    std::cout << "SchedulerModule stopped" << std::endl;

    impl_->running_ = false;

    if (impl_->schedulerThread_.joinable()) {
        impl_->schedulerThread_.join();
    }

    return true;
}

void SchedulerModule::cleanup() {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->jobs_.clear();
}

JobId SchedulerModule::scheduleCron(const std::string& name,
                                   const std::string& cronExpression,
                                   std::function<void()> task) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    JobId jobId = impl_->nextJobId_++;

    ScheduledJob job;
    job.jobId = jobId;
    job.name = name;
    job.type = JobType::CRON;
    job.status = JobStatus::PENDING;
    job.cronExpression = cronExpression;
    job.task = task;
    job.nextRunTime = std::chrono::system_clock::now();
    job.scheduledAt = std::chrono::system_clock::now();

    impl_->jobs_[jobId] = job;

    std::cout << "[Scheduler] Scheduled Cron job: " << name << " (" << cronExpression << ")" << std::endl;

    return jobId;
}

JobId SchedulerModule::scheduleDelayed(const std::string& name,
                                      std::chrono::seconds delay,
                                      std::function<void()> task) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    JobId jobId = impl_->nextJobId_++;

    ScheduledJob job;
    job.jobId = jobId;
    job.name = name;
    job.type = JobType::DELAYED;
    job.status = JobStatus::PENDING;
    job.task = task;
    job.scheduledAt = std::chrono::system_clock::now();
    job.nextRunTime = job.scheduledAt + delay;

    impl_->jobs_[jobId] = job;

    std::cout << "[Scheduler] Scheduled delayed job: " << name << " (" << delay.count() << "s)" << std::endl;

    return jobId;
}

JobId SchedulerModule::scheduleImmediate(const std::string& name,
                                        std::function<void()> task) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    JobId jobId = impl_->nextJobId_++;

    ScheduledJob job;
    job.jobId = jobId;
    job.name = name;
    job.type = JobType::IMMEDIATE;
    job.status = JobStatus::PENDING;
    job.task = task;
    job.scheduledAt = std::chrono::system_clock::now();
    job.nextRunTime = job.scheduledAt;

    impl_->jobs_[jobId] = job;

    std::cout << "[Scheduler] Scheduled immediate job: " << name << std::endl;

    return jobId;
}

bool SchedulerModule::cancel(const JobId& jobId) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->jobs_.find(jobId);
    if (it == impl_->jobs_.end()) {
        return false;
    }

    it->second.status = JobStatus::CANCELLED;
    impl_->jobs_.erase(it);

    std::cout << "[Scheduler] Cancelled job: " << jobId << std::endl;

    return true;
}

std::optional<ScheduledJob> SchedulerModule::getJob(const JobId& jobId) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->jobs_.find(jobId);
    if (it == impl_->jobs_.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<ScheduledJob> SchedulerModule::getAllJobs() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    std::vector<ScheduledJob> jobs;
    for (const auto& pair : impl_->jobs_) {
        jobs.push_back(pair.second);
    }

    return jobs;
}

std::vector<ScheduledJob> SchedulerModule::getRunningJobs() const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    std::vector<ScheduledJob> jobs;
    for (const auto& pair : impl_->jobs_) {
        if (pair.second.status == JobStatus::RUNNING) {
            jobs.push_back(pair.second);
        }
    }

    return jobs;
}

void SchedulerModule::executeJob(ScheduledJob& job) {
    if (job.task) {
        job.task();
    }
}

void SchedulerModule::updateNextRunTime(ScheduledJob& job) {
    impl_->updateNextRunTime(job);
}

} // namespace PaperCrawler
