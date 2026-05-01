#include "features/email/EmailService.hpp"

#include <sstream>
#include <fstream>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <array>

namespace PaperCrawler {

// ---------------------------------------------------------------------------
// Base64 helper (minimal, for Subject header encoding only)
// ---------------------------------------------------------------------------
static std::string base64Encode(const std::string& input) {
    static constexpr char kTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    out.reserve(4 * ((input.size() + 2) / 3));

    std::size_t i = 0;
    for (; i + 2 < input.size(); i += 3) {
        unsigned int n =
            (static_cast<unsigned char>(input[i])     << 16) |
            (static_cast<unsigned char>(input[i + 1]) << 8)  |
             static_cast<unsigned char>(input[i + 2]);
        out += kTable[(n >> 18) & 0x3F];
        out += kTable[(n >> 12) & 0x3F];
        out += kTable[(n >>  6) & 0x3F];
        out += kTable[ n        & 0x3F];
    }

    if (i < input.size()) {
        unsigned int n = static_cast<unsigned char>(input[i]) << 16;
        if (i + 1 < input.size()) {
            n |= static_cast<unsigned char>(input[i + 1]) << 8;
        }
        out += kTable[(n >> 18) & 0x3F];
        out += kTable[(n >> 12) & 0x3F];
        out += (i + 1 < input.size()) ? kTable[(n >> 6) & 0x3F] : '=';
        out += '=';
    }

    return out;
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

EmailService::EmailService() = default;

EmailService::EmailService(const SmtpConfig& config)
    : config_(config),
      configured_(!config.host.empty() && !config.username.empty()) {}

EmailService::~EmailService() = default;

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void EmailService::setConfig(const SmtpConfig& config) {
    config_ = config;
    configured_ = !config.host.empty() && !config.username.empty();
    if (configured_) {
        spdlog::info("[EmailService] SMTP configured: {}:{}", config.host, config.port);
    }
}

bool EmailService::isConfigured() const {
    return configured_;
}

// ---------------------------------------------------------------------------
// Plain send
// ---------------------------------------------------------------------------

EmailResult EmailService::send(const std::string& to,
                                const std::string& subject,
                                const std::string& body) {
    EmailResult result;

    if (!configured_) {
        result.success = false;
        result.errorMessage = "SMTP not configured";
        spdlog::warn("[EmailService] Cannot send email to {}: SMTP not configured", to);
        return result;
    }

    spdlog::info("[EmailService] Sending email to {}: {}", to, subject);

    std::string errorMsg;
    if (sendViaSmtp(to, subject, body, errorMsg)) {
        result.success = true;
        result.messageId = "msg_" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        spdlog::info("[EmailService] Email sent successfully to {}", to);
    } else {
        result.success = false;
        result.errorMessage = errorMsg;
        spdlog::error("[EmailService] Failed to send email to {}: {}", to, errorMsg);
    }

    return result;
}

// ---------------------------------------------------------------------------
// Template send
// ---------------------------------------------------------------------------

EmailResult EmailService::sendTemplate(const std::string& to,
                                        const std::string& subject,
                                        const std::string& templateName,
                                        const TemplateVars& vars) {
    std::string body = renderTemplate(templateName, vars);
    return send(to, subject, body);
}

// ---------------------------------------------------------------------------
// Template rendering (${variable} substitution)
// ---------------------------------------------------------------------------

std::string EmailService::renderTemplate(const std::string& templateName,
                                          const TemplateVars& vars) {
    std::string tmpl = getDefaultTemplate(templateName);

    for (const auto& [key, value] : vars) {
        std::string placeholder = "${" + key + "}";
        std::size_t pos = 0;
        while ((pos = tmpl.find(placeholder, pos)) != std::string::npos) {
            tmpl.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }

    return tmpl;
}

// ---------------------------------------------------------------------------
// Built-in email templates
// ---------------------------------------------------------------------------

std::string EmailService::getDefaultTemplate(const std::string& name) const {
    if (name == "password_reset") {
        return R"(<!DOCTYPE html><html><body style="font-family:sans-serif;padding:20px;">
<h2>Password Reset Request</h2>
<p>Hello ${username},</p>
<p>We received a request to reset your password.</p>
<p>Click the link below to reset your password (valid for ${expiry} minutes):</p>
<p><a href="${resetUrl}" style="padding:10px 20px;background:#4CAF50;color:white;text-decoration:none;border-radius:4px;">Reset Password</a></p>
<p>If you did not request this, please ignore this email.</p>
<p>— PaperCrawler Team</p>
</body></html>)";
    }

    if (name == "email_verification") {
        return R"(<!DOCTYPE html><html><body style="font-family:sans-serif;padding:20px;">
<h2>Email Verification</h2>
<p>Hello ${username},</p>
<p>Please verify your email address by clicking the link below:</p>
<p><a href="${verificationUrl}" style="padding:10px 20px;background:#2196F3;color:white;text-decoration:none;border-radius:4px;">Verify Email</a></p>
<p>This link will expire in ${expiry} minutes.</p>
<p>— PaperCrawler Team</p>
</body></html>)";
    }

    if (name == "welcome") {
        return R"(<!DOCTYPE html><html><body style="font-family:sans-serif;padding:20px;">
<h2>Welcome to PaperCrawler!</h2>
<p>Hello ${username},</p>
<p>Your account has been created successfully.</p>
<p>Start exploring papers, managing your research, and collaborating with peers.</p>
<p><a href="${loginUrl}" style="padding:10px 20px;background:#FF9800;color:white;text-decoration:none;border-radius:4px;">Get Started</a></p>
<p>— PaperCrawler Team</p>
</body></html>)";
    }

    if (name == "daily_digest") {
        return R"(<!DOCTYPE html><html><body style="font-family:sans-serif;padding:20px;">
<h2>Daily Research Digest</h2>
<p>Hello ${username},</p>
<p>Here's your research update for ${date}:</p>
<div style="background:#f5f5f5;padding:15px;border-radius:4px;">
${digestContent}
</div>
<p>— PaperCrawler Team</p>
</body></html>)";
    }

    // Unknown template -- return a minimal placeholder
    return "<html><body>" + name + "</body></html>";
}

// ---------------------------------------------------------------------------
// Low-level SMTP delivery via curl command
// ---------------------------------------------------------------------------

bool EmailService::sendViaSmtp(const std::string& to,
                                const std::string& subject,
                                const std::string& htmlBody,
                                std::string& errorMsg) {
    // Build the SMTP URL (smtps:// or smtp://)
    std::string smtpUrl = config_.useTls
        ? "smtps://" + config_.host + ":" + std::to_string(config_.port)
        : "smtp://"  + config_.host + ":" + std::to_string(config_.port);

    // Build the raw email (RFC 5322) with UTF-8 Base64 Subject
    std::ostringstream email;
    email << "From: " << config_.fromName << " <" << config_.fromEmail << ">\r\n";
    email << "To: <" << to << ">\r\n";
    email << "Subject: =?UTF-8?B?" << base64Encode(subject) << "?=\r\n";
    email << "MIME-Version: 1.0\r\n";
    email << "Content-Type: text/html; charset=UTF-8\r\n";
    email << "\r\n";
    email << htmlBody;

    // Write to a temporary .eml file
    std::string tmpFile = "/tmp/papercrawler_email_" +
        std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) +
        ".eml";

    {
        std::ofstream ofs(tmpFile);
        if (!ofs.is_open()) {
            errorMsg = "Cannot create temp email file: " + tmpFile;
            return false;
        }
        ofs << email.str();
    }

    // Assemble the curl command
    std::ostringstream cmd;
    cmd << "curl -s -S";
    if (config_.useTls) {
        cmd << " --ssl-reqd";
    }
    cmd << " --mail-from '" << config_.fromEmail << "'";
    cmd << " --mail-rcpt '" << to << "'";
    cmd << " -T '" << tmpFile << "'";
    cmd << " -u '" << config_.username << ":" << config_.password << "'";
    cmd << " --max-time " << config_.timeoutSeconds;
    cmd << " '" << smtpUrl << "'";
    cmd << " 2>&1";

    // Execute curl via popen
    FILE* pipe = popen(cmd.str().c_str(), "r");
    if (!pipe) {
        std::remove(tmpFile.c_str());
        errorMsg = "Failed to execute curl command";
        return false;
    }

    std::array<char, 256> buffer{};
    std::string output;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe)) {
        output += buffer.data();
    }
    int status = pclose(pipe);

    // Always clean up the temp file
    std::remove(tmpFile.c_str());

    if (status != 0) {
        errorMsg = "curl SMTP failed (exit " + std::to_string(status) + "): " + output;
        return false;
    }

    return true;
}

} // namespace PaperCrawler
