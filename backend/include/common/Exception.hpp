#pragma once

#include <string>
#include <exception>
#include <stdexcept>
#include <map>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

/**
 * @brief 应用错误码
 */
enum class ErrorCode {
    // 通用错误 (1000-1999)
    UNKNOWN = 1000,
    INTERNAL_ERROR = 1001,
    NOT_IMPLEMENTED = 1002,
    TIMEOUT = 1003,

    // 认证错误 (2000-2999)
    AUTH_FAILED = 2000,
    TOKEN_INVALID = 2001,
    TOKEN_EXPIRED = 2002,
    PERMISSION_DENIED = 2003,
    ACCOUNT_LOCKED = 2004,
    PASSWORD_WEAK = 2005,

    // 验证错误 (3000-3999)
    VALIDATION_ERROR = 3000,
    INVALID_INPUT = 3001,
    MISSING_FIELD = 3002,
    INVALID_FORMAT = 3003,
    DUPLICATE_ENTRY = 3004,

    // 资源错误 (4000-4999)
    NOT_FOUND = 4000,
    RESOURCE_CONFLICT = 4001,
    RESOURCE_LOCKED = 4002,
    QUOTA_EXCEEDED = 4003,

    // 数据库错误 (5000-5999)
    DATABASE_ERROR = 5000,
    QUERY_FAILED = 5001,
    CONNECTION_FAILED = 5002,
    TRANSACTION_FAILED = 5003,

    // 外部服务错误 (6000-6999)
    EXTERNAL_SERVICE_ERROR = 6000,
    RATE_LIMITED = 6001,
    SERVICE_UNAVAILABLE = 6002,

    // 业务逻辑错误 (7000-7999)
    BUSINESS_LOGIC_ERROR = 7000,
    INVALID_STATE = 7001,
    OPERATION_NOT_ALLOWED = 7002
};

/**
 * @brief 将错误码转换为字符串
 */
inline std::string errorCodeToString(ErrorCode code) {
    switch (code) {
        // 通用错误
        case ErrorCode::UNKNOWN: return "UNKNOWN";
        case ErrorCode::INTERNAL_ERROR: return "INTERNAL_ERROR";
        case ErrorCode::NOT_IMPLEMENTED: return "NOT_IMPLEMENTED";
        case ErrorCode::TIMEOUT: return "TIMEOUT";

        // 认证错误
        case ErrorCode::AUTH_FAILED: return "AUTH_FAILED";
        case ErrorCode::TOKEN_INVALID: return "TOKEN_INVALID";
        case ErrorCode::TOKEN_EXPIRED: return "TOKEN_EXPIRED";
        case ErrorCode::PERMISSION_DENIED: return "PERMISSION_DENIED";
        case ErrorCode::ACCOUNT_LOCKED: return "ACCOUNT_LOCKED";
        case ErrorCode::PASSWORD_WEAK: return "PASSWORD_WEAK";

        // 验证错误
        case ErrorCode::VALIDATION_ERROR: return "VALIDATION_ERROR";
        case ErrorCode::INVALID_INPUT: return "INVALID_INPUT";
        case ErrorCode::MISSING_FIELD: return "MISSING_FIELD";
        case ErrorCode::INVALID_FORMAT: return "INVALID_FORMAT";
        case ErrorCode::DUPLICATE_ENTRY: return "DUPLICATE_ENTRY";

        // 资源错误
        case ErrorCode::NOT_FOUND: return "NOT_FOUND";
        case ErrorCode::RESOURCE_CONFLICT: return "RESOURCE_CONFLICT";
        case ErrorCode::RESOURCE_LOCKED: return "RESOURCE_LOCKED";
        case ErrorCode::QUOTA_EXCEEDED: return "QUOTA_EXCEEDED";

        // 数据库错误
        case ErrorCode::DATABASE_ERROR: return "DATABASE_ERROR";
        case ErrorCode::QUERY_FAILED: return "QUERY_FAILED";
        case ErrorCode::CONNECTION_FAILED: return "CONNECTION_FAILED";
        case ErrorCode::TRANSACTION_FAILED: return "TRANSACTION_FAILED";

        // 外部服务错误
        case ErrorCode::EXTERNAL_SERVICE_ERROR: return "EXTERNAL_SERVICE_ERROR";
        case ErrorCode::RATE_LIMITED: return "RATE_LIMITED";
        case ErrorCode::SERVICE_UNAVAILABLE: return "SERVICE_UNAVAILABLE";

        // 业务逻辑错误
        case ErrorCode::BUSINESS_LOGIC_ERROR: return "BUSINESS_LOGIC_ERROR";
        case ErrorCode::INVALID_STATE: return "INVALID_STATE";
        case ErrorCode::OPERATION_NOT_ALLOWED: return "OPERATION_NOT_ALLOWED";

        default: return "UNKNOWN";
    }
}

/**
 * @brief 获取错误码对应的HTTP状态码
 */
inline int errorCodeToHttpStatus(ErrorCode code) {
    switch (code) {
        // 认证错误
        case ErrorCode::AUTH_FAILED:
        case ErrorCode::TOKEN_INVALID:
        case ErrorCode::PASSWORD_WEAK:
            return 401;

        case ErrorCode::PERMISSION_DENIED:
        case ErrorCode::ACCOUNT_LOCKED:
            return 403;

        // 验证错误
        case ErrorCode::VALIDATION_ERROR:
        case ErrorCode::INVALID_INPUT:
        case ErrorCode::MISSING_FIELD:
        case ErrorCode::INVALID_FORMAT:
            return 400;

        // 资源错误
        case ErrorCode::NOT_FOUND:
            return 404;

        case ErrorCode::RESOURCE_CONFLICT:
        case ErrorCode::DUPLICATE_ENTRY:
            return 409;

        case ErrorCode::RESOURCE_LOCKED:
            return 423;

        case ErrorCode::QUOTA_EXCEEDED:
            return 429;

        // 外部服务错误
        case ErrorCode::RATE_LIMITED:
            return 429;

        case ErrorCode::SERVICE_UNAVAILABLE:
        case ErrorCode::TIMEOUT:
            return 503;

        // 数据库错误
        case ErrorCode::DATABASE_ERROR:
        case ErrorCode::QUERY_FAILED:
        case ErrorCode::CONNECTION_FAILED:
        case ErrorCode::TRANSACTION_FAILED:
            return 500;

        // 其他
        case ErrorCode::NOT_IMPLEMENTED:
            return 501;

        default:
            return 500;
    }
}

/**
 * @brief 应用异常基类
 */
class AppException : public std::exception {
public:
    AppException(ErrorCode code, const std::string& message)
        : code_(code)
        , message_(message)
        , what_(formatWhat(code, message)) {}

    const char* what() const noexcept override {
        return what_.c_str();
    }

    ErrorCode getErrorCode() const noexcept { return code_; }
    const std::string& getMessage() const noexcept { return message_; }
    int getHttpStatus() const noexcept { return errorCodeToHttpStatus(code_); }

    /**
     * @brief 转换为JSON响应
     */
    nlohmann::json toJson() const {
        return {
            {"success", false},
            {"error", message_},
            {"error_code", errorCodeToString(code_)},
            {"http_status", getHttpStatus()}
        };
    }

protected:
    ErrorCode code_;
    std::string message_;
    std::string what_;

    static std::string formatWhat(ErrorCode code, const std::string& message) {
        return "[" + errorCodeToString(code_) + "] " + message;
    }
};

/**
 * @brief 认证异常
 */
class AuthException : public AppException {
public:
    explicit AuthException(const std::string& message, ErrorCode code = ErrorCode::AUTH_FAILED)
        : AppException(code, message) {}
};

/**
 * @brief 验证异常
 */
class ValidationException : public AppException {
public:
    explicit ValidationException(const std::string& message, ErrorCode code = ErrorCode::VALIDATION_ERROR)
        : AppException(code, message) {}

    ValidationException(const std::string& field, const std::string& message)
        : AppException(ErrorCode::VALIDATION_ERROR, field + ": " + message) {}
};

/**
 * @brief 资源未找到异常
 */
class NotFoundException : public AppException {
public:
    explicit NotFoundException(const std::string& resource)
        : AppException(ErrorCode::NOT_FOUND, resource + " not found") {}
};

/**
 * @brief 资源冲突异常
 */
class ConflictException : public AppException {
public:
    explicit ConflictException(const std::string& message)
        : AppException(ErrorCode::RESOURCE_CONFLICT, message) {}
};

/**
 * @brief 数据库异常
 */
class DatabaseException : public AppException {
public:
    explicit DatabaseException(const std::string& message, ErrorCode code = ErrorCode::DATABASE_ERROR)
        : AppException(code, message) {}

    DatabaseException(const std::string& message, const std::string& sql)
        : AppException(ErrorCode::DATABASE_ERROR, message + " [SQL: " + sql + "]") {}
};

/**
 * @brief 外部服务异常
 */
class ExternalServiceException : public AppException {
public:
    explicit ExternalServiceException(const std::string& service, const std::string& message,
                                     ErrorCode code = ErrorCode::EXTERNAL_SERVICE_ERROR)
        : AppException(code, "[" + service + "] " + message) {}
};

/**
 * @brief 业务逻辑异常
 */
class BusinessException : public AppException {
public:
    explicit BusinessException(const std::string& message, ErrorCode code = ErrorCode::BUSINESS_LOGIC_ERROR)
        : AppException(code, message) {}
};

/**
 * @brief 异常处理助手
 */
class ExceptionHandler {
public:
    /**
     * @brief 将异常转换为HTTP响应
     */
    static nlohmann::json toErrorResponse(const std::exception& e) {
        // 尝试转换为AppException
        if (const auto* appEx = dynamic_cast<const AppException*>(&e)) {
            return appEx->toJson();
        }

        // 处理JSON解析异常
        if (const auto* jsonEx = dynamic_cast<const nlohmann::json::exception*>(&e)) {
            return {
                {"success", false},
                {"error", "Invalid JSON format"},
                {"error_code", "INVALID_JSON"},
                {"http_status", 400}
            };
        }

        // 处理标准异常
        if (typeid(e) == typeid(std::runtime_error) ||
            typeid(e) == typeid(std::logic_error)) {
            return {
                {"success", false},
                {"error", e.what()},
                {"error_code", "RUNTIME_ERROR"},
                {"http_status", 500}
            };
        }

        // 未知异常
        return {
            {"success", false},
            {"error", "An unexpected error occurred"},
            {"error_code", "UNKNOWN"},
            {"http_status", 500}
        };
    }

    /**
     * @brief 安全地执行函数并捕获异常
     */
    template<typename F>
    static auto tryExecute(F&& func) -> decltype(func()) {
        try {
            return func();
        } catch (const AppException& e) {
            throw; // 重新抛出应用异常
        } catch (const std::exception& e) {
            throw AppException(ErrorCode::INTERNAL_ERROR, e.what());
        } catch (...) {
            throw AppException(ErrorCode::INTERNAL_ERROR, "Unknown error occurred");
        }
    }

    /**
     * @brief 记录异常
     */
    static void logException(const std::exception& e, const std::string& context = "") {
        if (const auto* appEx = dynamic_cast<const AppException*>(&e)) {
            spdlog::error("{} [{}] {}", context, errorCodeToString(appEx->getErrorCode()), e.what());
        } else {
            spdlog::error("{} {}", context, e.what());
        }
    }
};

/**
 * @brief 异常处理宏
 */
#define TRY_EXECUTE(func) \
    ExceptionHandler::tryExecute([&]() { return func(); })

#define THROW_IF(condition, exception_type, message) \
    do { \
        if (condition) { \
            throw exception_type(message); \
        } \
    } while (0)

#define THROW_NOT_NULL(ptr, message) \
    THROW_IF((ptr) == nullptr, std::runtime_error, message)

#define THROW_IF_NOT(condition, exception_type, message) \
    THROW_IF(!(condition), exception_type, message)

} // namespace PaperCrawler
