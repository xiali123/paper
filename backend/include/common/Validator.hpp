#pragma once

#include <string>
#include <vector>
#include <regex>
#include <optional>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

/**
 * @brief 输入验证器
 *
 * 提供常用的输入验证方法
 */
class Validator {
public:
    // 验证结果
    struct ValidationResult {
        bool valid{true};
        std::string errorMessage;
        std::string field;

        static ValidationResult ok() {
            return ValidationResult{true, "", ""};
        }

        static ValidationResult error(const std::string& message,
                                     const std::string& field = "") {
            return ValidationResult{false, message, field};
        }
    };

    /**
     * @brief 验证用户名
     * - 长度：3-32字符
     * - 允许：字母、数字、下划线
     * - 必须以字母开头
     */
    static ValidationResult validateUsername(const std::string& username) {
        if (username.empty()) {
            return ValidationResult::error("Username is required", "username");
        }

        if (username.length() < 3) {
            return ValidationResult::error("Username must be at least 3 characters", "username");
        }

        if (username.length() > 32) {
            return ValidationResult::error("Username must not exceed 32 characters", "username");
        }

        std::regex pattern("^[a-zA-Z][a-zA-Z0-9_]*$");
        if (!std::regex_match(username, pattern)) {
            return ValidationResult::error(
                "Username must start with a letter and contain only letters, numbers, and underscores",
                "username"
            );
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 验证邮箱
     */
    static ValidationResult validateEmail(const std::string& email) {
        if (email.empty()) {
            return ValidationResult::error("Email is required", "email");
        }

        if (email.length() > 255) {
            return ValidationResult::error("Email must not exceed 255 characters", "email");
        }

        // 简单的邮箱验证（RFC 5322的简化版）
        std::regex pattern(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        if (!std::regex_match(email, pattern)) {
            return ValidationResult::error("Invalid email format", "email");
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 验证密码
     * - 最小长度：8字符
     * - 建议包含：大写字母、小写字母、数字、特殊字符
     */
    static ValidationResult validatePassword(const std::string& password,
                                            bool strict = false) {
        if (password.empty()) {
            return ValidationResult::error("Password is required", "password");
        }

        if (password.length() < 8) {
            return ValidationResult::error("Password must be at least 8 characters", "password");
        }

        if (password.length() > 128) {
            return ValidationResult::error("Password must not exceed 128 characters", "password");
        }

        if (strict) {
            bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;

            for (char c : password) {
                if (isupper(c)) hasUpper = true;
                else if (islower(c)) hasLower = true;
                else if (isdigit(c)) hasDigit = true;
                else if (ispunct(c)) hasSpecial = true;
            }

            std::vector<std::string> missing;
            if (!hasUpper) missing.push_back("uppercase letter");
            if (!hasLower) missing.push_back("lowercase letter");
            if (!hasDigit) missing.push_back("digit");
            if (!hasSpecial) missing.push_back("special character");

            if (!missing.empty()) {
                std::string msg = "Password must contain: ";
                for (size_t i = 0; i < missing.size(); ++i) {
                    if (i > 0) msg += ", ";
                    msg += missing[i];
                }
                return ValidationResult::error(msg, "password");
            }
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 验证ID（正整数）
     */
    static ValidationResult validateId(const std::string& idStr, const std::string& fieldName = "id") {
        if (idStr.empty()) {
            return ValidationResult::error(fieldName + " is required", fieldName);
        }

        try {
            int id = std::stoi(idStr);
            if (id <= 0) {
                return ValidationResult::error(fieldName + " must be a positive integer", fieldName);
            }
        } catch (const std::exception&) {
            return ValidationResult::error("Invalid " + fieldName + " format", fieldName);
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 验证分页参数
     */
    static ValidationResult validatePagination(int page, int limit) {
        if (page < 1) {
            return ValidationResult::error("Page must be at least 1", "page");
        }

        if (page > 10000) {
            return ValidationResult::error("Page must not exceed 10000", "page");
        }

        if (limit < 1) {
            return ValidationResult::error("Limit must be at least 1", "limit");
        }

        if (limit > 100) {
            return ValidationResult::error("Limit must not exceed 100", "limit");
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 验证URL
     */
    static ValidationResult validateUrl(const std::string& url, const std::string& fieldName = "url") {
        if (url.empty()) {
            return ValidationResult::error(fieldName + " is required", fieldName);
        }

        if (url.length() > 2048) {
            return ValidationResult::error(fieldName + " must not exceed 2048 characters", fieldName);
        }

        std::regex pattern(R"(^https?://[^\s/$.?#].[^\s]*$)");
        if (!std::regex_match(url, pattern)) {
            return ValidationResult::error("Invalid URL format", fieldName);
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 验证字符串长度
     */
    static ValidationResult validateStringLength(const std::string& str,
                                                  int minLength,
                                                  int maxLength,
                                                  const std::string& fieldName) {
        if (str.length() < static_cast<size_t>(minLength)) {
            return ValidationResult::error(
                fieldName + " must be at least " + std::to_string(minLength) + " characters",
                fieldName
            );
        }

        if (str.length() > static_cast<size_t>(maxLength)) {
            return ValidationResult::error(
                fieldName + " must not exceed " + std::to_string(maxLength) + " characters",
                fieldName
            );
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 验证JSON必填字段
     */
    static ValidationResult validateRequiredFields(
        const nlohmann::json& json,
        const std::vector<std::string>& fields) {

        for (const auto& field : fields) {
            if (!json.contains(field)) {
                return ValidationResult::error("Field '" + field + "' is required", field);
            }

            if (json[field].is_null()) {
                return ValidationResult::error("Field '" + field + "' cannot be null", field);
            }
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 验证枚举值
     */
    template<typename T>
    static ValidationResult validateEnum(const std::string& value,
                                        const std::string& fieldName,
                                        const std::map<std::string, T>& validValues) {
        if (validValues.find(value) == validValues.end()) {
            std::string allowed;
            for (const auto& [key, val] : validValues) {
                if (!allowed.empty()) allowed += ", ";
                allowed += key;
            }
            return ValidationResult::error(
                "Invalid " + fieldName + ". Allowed values: " + allowed,
                fieldName
            );
        }

        return ValidationResult::ok();
    }

    /**
     * @brief 清理和转义字符串（防止XSS）
     */
    static std::string sanitizeString(const std::string& input) {
        std::string result;
        result.reserve(input.size() * 1.2);

        for (char c : input) {
            switch (c) {
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '&': result += "&amp;"; break;
                case '"': result += "&quot;"; break;
                case '\'': result += "&#39;"; break;
                default: result += c; break;
            }
        }

        return result;
    }

    /**
     * @brief 验证并清理SQL字符串（防止SQL注入）
     * 注意：这应该配合使用参数化查询，而不是替代
     */
    static std::string escapeSqlString(const std::string& input) {
        std::string result;
        result.reserve(input.length() * 2);

        for (char c : input) {
            switch (c) {
                case '\'': result.append("\\'"); break;
                case '\"': result.append("\\\""); break;
                case '\\': result.append("\\\\"); break;
                case '\n': result.append("\\n"); break;
                case '\r': result.append("\\r"); break;
                case '\t': result.append("\\t"); break;
                case '\0': result.append("\\0"); break;
                default: result.push_back(c); break;
            }
        }

        return result;
    }

    /**
     * @brief 批量验证
     */
    static std::vector<ValidationResult> validateAll(
        const std::vector<std::function<ValidationResult()>>& validators) {

        std::vector<ValidationResult> results;
        for (const auto& validator : validators) {
            auto result = validator();
            results.push_back(result);
            if (!result.valid) {
                // 遇到第一个错误就停止（可以修改为收集所有错误）
                break;
            }
        }
        return results;
    }
};

} // namespace PaperCrawler
