#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief 通知渠道
 */
enum class NotificationChannel {
    EMAIL,
    SMS,
    PUSH,
    WEBHOOK,
    SLACK,
    TELEGRAM,
    DISCORD,
    CUSTOM
};

/**
 * @brief 优先级
 */
enum class NotificationPriority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    URGENT = 3
};

/**
 * @brief 通知状态
 */
enum class NotificationStatus {
    PENDING,
    SENDING,
    SENT,
    FAILED,
    RETRYING
};

/**
 * @brief 通知消息
 */
struct NotificationMessage {
    std::string messageId;
    std::string to;
    std::string subject;       // 标题
    std::string body;          // 内容
    std::string htmlBody;      // HTML内容（可选）
    NotificationChannel channel;
    NotificationPriority priority{NotificationPriority::NORMAL};
    NotificationStatus status{NotificationStatus::PENDING};

    // 元数据
    std::map<std::string, std::string> metadata;
    std::vector<std::string> attachments;  // 附件路径
    std::map<std::string, std::string> headers;  // 自定义头

    // 重试配置
    int maxRetries{3};
    int retryCount{0};
    std::chrono::seconds retryDelay{5};

    // 时间戳
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point sentAt;
    std::chrono::system_clock::time_point lastAttemptAt;

    // 错误信息
    std::string errorMessage;

    /**
     * @brief 生成消息ID
     */
    static std::string generateId() {
        static std::atomic<uint64_t> counter{0};
        return "msg_" + std::to_string(std::time(nullptr)) + "_" +
               std::to_string(counter.fetch_add(1));
    }
};

/**
 * @brief 通知发送结果
 */
struct NotificationResult {
    bool success{false};
    std::string messageId;
    std::string errorMessage;
    std::chrono::system_clock::time_point sentAt;
};

/**
 * @brief 邮件配置
 */
struct EmailConfig {
    std::string smtpHost;
    int smtpPort{587};
    bool useTLS{true};
    std::string username;
    std::string password;
    std::string fromAddress;
    std::string fromName;
};

/**
 * @brief SMS配置
 */
struct SMSConfig {
    std::string provider;      // twilio, nexmo, etc.
    std::string apiKey;
    std::string apiSecret;
    std::string fromNumber;
};

/**
 * @brief Push通知配置
 */
struct PushConfig {
    std::string provider;      // fcm, apns, etc.
    std::string apiKey;
    std::string certificatePath;
    bool useSandbox{false};
};

/**
 * @brief Webhook配置
 */
struct WebhookConfig {
    std::string url;
    std::string method{"POST"};
    std::map<std::string, std::string> headers;
    int timeoutSeconds{30};
    bool retryOnFailure{true};
};

/**
 * @brief 通知统计
 */
struct NotificationStats {
    uint64_t totalSent{0};
    uint64_t totalFailed{0};
    uint64_t totalRetries{0};
    std::map<NotificationChannel, uint64_t> sentByChannel;
    std::map<NotificationChannel, uint64_t> failedByChannel;
    std::chrono::system_clock::time_point lastSentAt;
    size_t queueDepth{0};
};

/**
 * @brief 通知模块配置
 */
struct NotificationConfig {
    bool enableEmail{true};
    bool enableSMS{false};
    bool enablePush{false};
    bool enableWebhook{false};
    bool queueEnabled{true};
    size_t queueSize{10000};
    int workerThreads{4};
    std::chrono::seconds defaultRetryDelay{5};
    int defaultMaxRetries{3};
};

/**
 * @brief 通知模块
 *
 * 功能：
 * 1. 多渠道发送（邮件、短信、推送、Webhook）
 * 2. 异步发送（队列）
 * 3. 重试机制
 * 4. 模板管理
 * 5. 优先级队列
 * 6. 批量发送
 * 7. 发送统计
 *
 * 特性：
 * - 高可靠：失败自动重试
 * - 高性能：异步队列处理
 * - 灵活：支持多种通知渠道
 * - 模板：支持变量替换
 * - 监控：完整的发送统计
 */
class NotificationModule : public IModule {
public:
    NotificationModule();
    ~NotificationModule() override;

    std::string getName() const override { return "Notification"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Multi-channel notification service (Email/SMS/Push/Webhook)";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 发送邮件
     */
    NotificationResult sendEmail(const NotificationMessage& message);

    /**
     * @brief 发送短信
     */
    NotificationResult sendSMS(const NotificationMessage& message);

    /**
     * @brief 发送Push通知
     */
    NotificationResult sendPush(const NotificationMessage& message);

    /**
     * @brief 发送Webhook
     */
    NotificationResult sendWebhook(const NotificationMessage& message);

    /**
     * @brief 通用发送方法
     */
    NotificationResult send(const NotificationMessage& message);

    /**
     * @brief 批量发送
     */
    std::vector<NotificationResult> sendBatch(const std::vector<NotificationMessage>& messages);

    /**
     * @brief 异步发送
     */
    void sendAsync(const NotificationMessage& message,
                  std::function<void(const NotificationResult&)> callback = nullptr);

    /**
     * @brief 使用模板发送
     */
    NotificationResult sendWithTemplate(const std::string& templateName,
                                       const std::map<std::string, std::string>& variables,
                                       const std::string& to,
                                       NotificationChannel channel);

    /**
     * @brief 注册模板
     */
    void registerTemplate(const std::string& name,
                         const std::string& subjectTemplate,
                         const std::string& bodyTemplate);

    /**
     * @brief 设置邮件配置
     */
    void setEmailConfig(const EmailConfig& config);

    /**
     * @brief 设置SMS配置
     */
    void setSMSConfig(const SMSConfig& config);

    /**
     * @brief 设置Push配置
     */
    void setPushConfig(const PushConfig& config);

    /**
     * @brief 设置Webhook配置
     */
    void setWebhookConfig(const WebhookConfig& config);

    /**
     * @brief 获取通知统计
     */
    NotificationStats getStats() const;

    /**
     * @brief 设置模块配置
     */
    void setConfig(const NotificationConfig& config);

    /**
     * @brief 清空队列
     */
    void clearQueue();

    /**
     * @brief 获取队列深度
     */
    size_t getQueueDepth() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    void workerLoop();
    NotificationResult doSend(const NotificationMessage& message);
    bool shouldRetry(const NotificationMessage& message);

    NotificationConfig config_;
    EmailConfig emailConfig_;
    SMSConfig smsConfig_;
    PushConfig pushConfig_;
    WebhookConfig webhookConfig_;

    // 模板存储
    struct Template {
        std::string subjectTemplate;
        std::string bodyTemplate;
    };
    std::map<std::string, Template> templates_;

    // 队列和工作线程
    std::queue<NotificationMessage> messageQueue_;
    std::vector<std::thread> workerThreads_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
    std::condition_variable condition_;

    NotificationStats stats_;
};

} // namespace PaperCrawler
