#include "features/operations/NotificationModule.hpp"
#include <iostream>
#include <thread>
#include <sstream>

namespace PaperCrawler {

// ============================================================================
// NotificationModule 实现
// ============================================================================

class NotificationModule::Impl {
public:
    std::queue<NotificationMessage> emailQueue_;
    std::queue<NotificationMessage> smsQueue_;
    std::queue<NotificationMessage> pushQueue_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> running_{false};
    NotificationStats stats_{};

    void emailWorker() {
        while (running_) {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this] {
                return !emailQueue_.empty() || !running_;
            });

            if (!running_) break;

            while (!emailQueue_.empty()) {
                auto msg = emailQueue_.front();
                emailQueue_.pop();
                lock.unlock();

                // 发送邮件
                bool success = sendEmail(msg);

                lock.lock();
                stats_.totalSent++;
                if (success) {
                    stats_.sentByChannel[NotificationChannel::EMAIL]++;
                } else {
                    stats_.totalFailed++;
                    stats_.failedByChannel[NotificationChannel::EMAIL]++;
                }
            }
        }
    }

    bool sendEmail(const NotificationMessage& msg) {
        // TODO: 实现邮件发送逻辑
        std::cout << "[Email] To: " << msg.to
                  << " | Subject: " << msg.subject << std::endl;
        return true;
    }

    bool sendSMS(const NotificationMessage& msg) {
        // TODO: 实现短信发送逻辑
        std::cout << "[SMS] To: " << msg.to
                  << " | Message: " << msg.body << std::endl;
        return true;
    }

    bool sendPush(const NotificationMessage& msg) {
        // TODO: 实现推送发送逻辑
        std::cout << "[Push] To: " << msg.to
                  << " | Message: " << msg.body << std::endl;
        return true;
    }
};

NotificationModule::NotificationModule()
    : impl_(std::make_unique<Impl>()) {}

NotificationModule::~NotificationModule() {
    stop();
}

bool NotificationModule::initialize() {
    std::cout << "NotificationModule::initialize" << std::endl;
    return true;
}

bool NotificationModule::start() {
    std::cout << "NotificationModule started" << std::endl;

    impl_->running_ = true;
    impl_->workerThreads_.push_back(
        std::thread(&NotificationModule::Impl::emailWorker, impl_.get())
    );

    return true;
}

bool NotificationModule::stop() {
    std::cout << "NotificationModule stopped" << std::endl;

    impl_->running_ = false;
    impl_->condition_.notify_all();

    for (auto& thread : impl_->workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    impl_->workerThreads_.clear();

    return true;
}

void NotificationModule::cleanup() {
    // 清理资源
}

NotificationResult NotificationModule::sendEmail(const NotificationMessage& message) {
    NotificationResult result;
    result.success = true;
    result.sentAt = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->emailQueue_.push(message);
    impl_->condition_.notify_one();

    return result;
}

NotificationResult NotificationModule::sendSMS(const NotificationMessage& message) {
    NotificationResult result;
    result.success = true;
    result.sentAt = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(impl_->mutex_);
    bool success = impl_->sendSMS(message);
    impl_->stats_.totalSent++;
    if (success) {
        impl_->stats_.sentByChannel[NotificationChannel::SMS]++;
    } else {
        impl_->stats_.totalFailed++;
        impl_->stats_.failedByChannel[NotificationChannel::SMS]++;
    }
    result.success = success;

    return result;
}

NotificationResult NotificationModule::sendPush(const NotificationMessage& message) {
    NotificationResult result;
    result.success = true;
    result.sentAt = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->pushQueue_.push(message);
    bool success = impl_->sendPush(message);
    impl_->stats_.totalSent++;
    if (success) {
        impl_->stats_.sentByChannel[NotificationChannel::PUSH]++;
    } else {
        impl_->stats_.totalFailed++;
        impl_->stats_.failedByChannel[NotificationChannel::PUSH]++;
    }
    result.success = success;

    return result;
}

std::vector<NotificationResult> NotificationModule::sendBatch(
    const std::vector<NotificationMessage>& messages) {

    std::vector<NotificationResult> results;
    for (const auto& msg : messages) {
        std::ostringstream oss;
        oss << "msg_" << std::time(nullptr) << "_" << results.size();
        std::string messageId = oss.str();

        NotificationResult result;
        result.messageId = messageId;
        result.success = true;
        result.sentAt = std::chrono::system_clock::now();

        // 根据消息类型发送
        auto it = msg.metadata.find("type");
        if (it != msg.metadata.end() && it->second == "email") {
            result = sendEmail(msg);
        } else if (it != msg.metadata.end() && it->second == "sms") {
            result = sendSMS(msg);
        } else if (it != msg.metadata.end() && it->second == "push") {
            result = sendPush(msg);
        }

        results.push_back(result);
    }

    return results;
}

NotificationStats NotificationModule::getStats() const {
    return impl_->stats_;
}

} // namespace PaperCrawler
