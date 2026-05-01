#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <spdlog/spdlog.h>

namespace PaperCrawler {

// 邮件发送结果
struct EmailResult {
    bool success = false;
    std::string errorMessage;
    std::string messageId;
};

// 邮件模板变量
using TemplateVars = std::map<std::string, std::string>;

// SMTP配置
struct SmtpConfig {
    std::string host = "smtp.gmail.com";
    int port = 587;
    std::string username;
    std::string password;
    std::string fromName = "PaperCrawler";
    std::string fromEmail;
    bool useTls = true;
    int timeoutSeconds = 30;
};

// 邮件服务 — SMTP via libcurl
class EmailService {
public:
    EmailService();
    explicit EmailService(const SmtpConfig& config);
    ~EmailService();

    // 发送邮件
    EmailResult send(const std::string& to,
                     const std::string& subject,
                     const std::string& body);

    // 发送模板邮件
    EmailResult sendTemplate(const std::string& to,
                              const std::string& subject,
                              const std::string& templateName,
                              const TemplateVars& vars);

    // 配置
    void setConfig(const SmtpConfig& config);
    const SmtpConfig& getConfig() const { return config_; }

    // 健康检查
    bool isConfigured() const;

private:
    SmtpConfig config_;
    bool configured_{false};

    // 模板渲染
    std::string renderTemplate(const std::string& templateName, const TemplateVars& vars);

    // 获取默认模板
    std::string getDefaultTemplate(const std::string& name) const;

    // SMTP发送（通过外部命令或curl）
    bool sendViaSmtp(const std::string& to, const std::string& subject,
                     const std::string& htmlBody, std::string& errorMsg);
};

} // namespace PaperCrawler
