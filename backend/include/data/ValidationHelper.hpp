#pragma once

#include <string>
#include <vector>
#include <regex>
#include <algorithm>
#include <sstream>
#include <cmath>

namespace PaperCrawler {

// 验证结果
struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;

    void addError(const std::string& field, const std::string& message) {
        valid = false;
        errors.push_back(field + ": " + message);
    }

    std::string firstError() const {
        return errors.empty() ? "" : errors[0];
    }

    std::string toString() const {
        std::ostringstream ss;
        for (size_t i = 0; i < errors.size(); ++i) {
            if (i > 0) ss << "; ";
            ss << errors[i];
        }
        return ss.str();
    }
};

// 验证规则
class ValidationHelper {
public:
    // 必填
    static void required(ValidationResult& vr, const std::string& field,
                         const std::string& value) {
        if (value.empty()) {
            vr.addError(field, "不能为空");
        }
    }

    // 可选字符串长度
    static void minLength(ValidationResult& vr, const std::string& field,
                          const std::string& value, size_t minLen) {
        if (!value.empty() && value.length() < minLen) {
            vr.addError(field, "长度不能少于" + std::to_string(minLen) + "个字符");
        }
    }

    static void maxLength(ValidationResult& vr, const std::string& field,
                          const std::string& value, size_t maxLen) {
        if (!value.empty() && value.length() > maxLen) {
            vr.addError(field, "长度不能超过" + std::to_string(maxLen) + "个字符");
        }
    }

    // 邮箱格式
    static bool isEmail(const std::string& email) {
        if (email.empty()) return false;
        static const std::regex pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        return std::regex_match(email, pattern);
    }

    static void validateEmail(ValidationResult& vr, const std::string& field,
                              const std::string& email) {
        if (email.empty()) {
            vr.addError(field, "邮箱不能为空");
        } else if (!isEmail(email)) {
            vr.addError(field, "邮箱格式无效");
        }
    }

    // 用户名 (字母数字下划线，3-32字符)
    static void validateUsername(ValidationResult& vr, const std::string& username) {
        if (username.empty()) {
            vr.addError("username", "用户名不能为空");
        } else if (username.length() < 3 || username.length() > 32) {
            vr.addError("username", "用户名长度须在3-32之间");
        } else {
            static const std::regex pattern(R"(^[a-zA-Z0-9_]+$)");
            if (!std::regex_match(username, pattern)) {
                vr.addError("username", "用户名只能包含字母、数字和下划线");
            }
        }
    }

    // 密码强度 (最少8位，含大小写+数字)
    static void validatePassword(ValidationResult& vr, const std::string& password) {
        if (password.empty()) {
            vr.addError("password", "密码不能为空");
        } else if (password.length() < 8) {
            vr.addError("password", "密码长度不能少于8位");
        } else {
            bool hasUpper = false, hasLower = false, hasDigit = false;
            for (char c : password) {
                if (c >= 'A' && c <= 'Z') hasUpper = true;
                else if (c >= 'a' && c <= 'z') hasLower = true;
                else if (c >= '0' && c <= '9') hasDigit = true;
            }
            if (!hasUpper) vr.addError("password", "密码须包含大写字母");
            if (!hasLower) vr.addError("password", "密码须包含小写字母");
            if (!hasDigit) vr.addError("password", "密码须包含数字");
        }
    }

    // 整数范围
    static void validateIntRange(ValidationResult& vr, const std::string& field,
                                 int value, int minVal, int maxVal) {
        if (value < minVal || value > maxVal) {
            vr.addError(field, "值须在" + std::to_string(minVal) + "-" + std::to_string(maxVal) + "之间");
        }
    }

    // 正整数ID
    static void validateId(ValidationResult& vr, const std::string& field,
                           const std::string& idStr) {
        if (idStr.empty()) {
            vr.addError(field, "ID不能为空");
            return;
        }
        try {
            int id = std::stoi(idStr);
            if (id <= 0) {
                vr.addError(field, "ID须为正整数");
            }
        } catch (...) {
            vr.addError(field, "ID格式无效");
        }
    }

    // URL格式
    static bool isUrl(const std::string& url) {
        if (url.empty()) return false;
        static const std::regex pattern(R"(^https?://[^\s/$.?#].[^\s]*$)");
        return std::regex_match(url, pattern);
    }

    // 安全字符串（防注入基础检查）
    static std::string sanitize(const std::string& input) {
        std::string result;
        result.reserve(input.size());
        for (char c : input) {
            switch (c) {
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '"': result += "&quot;"; break;
                case '\'': result += "&#39;"; break;
                case '&': result += "&amp;"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    // 枚举值检查
    template<typename... Args>
    static bool isOneOf(const std::string& value, Args... args) {
        return ((value == args) || ...);
    }
};

} // namespace PaperCrawler
