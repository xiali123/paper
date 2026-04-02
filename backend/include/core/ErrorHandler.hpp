// 统一错误处理机制
// 文件位置：backend/include/core/ErrorHandler.hpp

#pragma once

#include <string>
#include <stdexcept>
#include <memory>
#include <functional>
#include <map>

namespace PaperCrawler {

/**
 * @brief 错误代码枚举
 */
enum class ErrorCode {
    // 通用错误 (0-999)
    UNKNOWN = 0,
    SUCCESS = 1,

    // 客户端错误 (1000-1999)
    BAD_REQUEST = 1000,
    UNAUTHORIZED = 1001,
    FORBIDDEN = 1002,
    NOT_FOUND = 1003,
    CONFLICT = 1004,
    VALIDATION_ERROR = 1005,
    RATE_LIMIT_EXCEEDED = 1006,

    // 服务器错误 (2000-2999)
    INTERNAL_ERROR = 2000,
    DATABASE_ERROR = 2001,
    NETWORK_ERROR = 2002,
    TIMEOUT_ERROR = 2003,
    SERVICE_UNAVAILABLE = 2004,

    // 业务错误 (3000-3999)
    PAPER_NOT_FOUND = 3000,
    USER_NOT_FOUND = 3001,
    TEMPLATE_NOT_FOUND = 3002,
    CRAWL_FAILED = 3003,
    PARSE_ERROR = 3004,
    DUPLICATE_PAPER = 3005,
    INVALID_CREDENTIALS = 3006,

    // 安全错误 (4000-4999)
    SQL_INJECTION_DETECTED = 4000,
    XSS_DETECTED = 4001,
    CSRF_DETECTED = 4002,
    AUTHENTICATION_FAILED = 4003,
    PERMISSION_DENIED = 4004
};

/**
 * @brief 错误严重级别
 */
enum class ErrorSeverity {
    INFO,       // 信息性
    WARNING,    // 警告
    ERROR,      // 错误
    CRITICAL,   // 严重
    FATAL       // 致命
};

/**
 * @brief 应用异常类
 */
class AppException : public std::exception {
public:
    AppException(ErrorCode code, const std::string& message)
        : code_(code), message_(message), severity_(ErrorSeverity::ERROR) {}

    AppException(ErrorCode code, const std::string& message, ErrorSeverity severity)
        : code_(code), message_(message), severity_(severity) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    ErrorCode getCode() const { return code_; }
    ErrorSeverity getSeverity() const { return severity_; }
    const std::string& getMessage() const { return message_; }

    /**
     * @brief 获取HTTP状态码
     */
    int getHttpStatusCode() const {
        switch (code_) {
            case ErrorCode::SUCCESS:
                return 200;
            case ErrorCode::BAD_REQUEST:
            case ErrorCode::VALIDATION_ERROR:
                return 400;
            case ErrorCode::UNAUTHORIZED:
            case ErrorCode::INVALID_CREDENTIALS:
            case ErrorCode::AUTHENTICATION_FAILED:
                return 401;
            case ErrorCode::FORBIDDEN:
            case ErrorCode::PERMISSION_DENIED:
                return 403;
            case ErrorCode::NOT_FOUND:
            case ErrorCode::PAPER_NOT_FOUND:
            case ErrorCode::USER_NOT_FOUND:
            case ErrorCode::TEMPLATE_NOT_FOUND:
                return 404;
            case ErrorCode::CONFLICT:
            case ErrorCode::DUPLICATE_PAPER:
                return 409;
            case ErrorCode::INTERNAL_ERROR:
            case ErrorCode::DATABASE_ERROR:
            case ErrorCode::NETWORK_ERROR:
                return 500;
            case ErrorCode::SERVICE_UNAVAILABLE:
                return 503;
            default:
                return 500;
        }
    }

private:
    ErrorCode code_;
    std::string message_;
    ErrorSeverity severity_;
};

/**
 * @brief 错误处理策略
 */
enum class ErrorHandlingStrategy {
    LOG_AND_CONTINUE,    // 记录日志并继续
    LOG_AND_THROW,       // 记录日志并抛出异常
    RETRY,              // 重试
    FALLBACK,           // 降级处理
    CIRCUIT_BREAKER      // 熔断
};

/**
 * @brief 错误处理器接口
 */
class IErrorHandler {
public:
    virtual ~IErrorHandler() = default;

    /**
     * @brief 处理错误
     * @param error 异常对象
     * @param context 上下文信息
     * @return 是否已处理（true表示不应该继续传播）
     */
    virtual bool handleError(
        const std::exception& error,
        const std::map<std::string, std::string>& context = {}
    ) = 0;
};

/**
 * @brief 统一错误处理器
 */
class ErrorHandler {
public:
    /**
     * @brief 构造函数
     */
    ErrorHandler();

    /**
     * @brief 处理异常
     * @param error 异常对象
     * @param context 上下文信息
     */
    void handle(
        const std::exception& error,
        const std::map<std::string, std::string>& context = {}
    );

    /**
     * @brief 处理异常并返回错误响应
     * @param error 异常对象
     * @return JSON错误响应
     */
    std::string handleAndRespond(const std::exception& error);

    /**
     * @brief 注册错误处理器
     * @param errorCode 错误代码
     * @param handler 处理器
     */
    void registerHandler(ErrorCode errorCode, std::shared_ptr<IErrorHandler> handler);

    /**
     * @brief 设置默认处理策略
     */
    void setDefaultStrategy(ErrorHandlingStrategy strategy);

    /**
     * @brief 启用/禁用错误详情（生产环境应禁用）
     */
    void setDetailedErrors(bool enabled);

private:
    std::map<ErrorCode, std::shared_ptr<IErrorHandler>> handlers_;
    ErrorHandlingStrategy defaultStrategy_;
    bool detailedErrors_{true};
};

/**
 * @brief 全局错误处理器（单例）
 */
 ErrorHandler& getGlobalErrorHandler();

/**
 * @brief 错误处理宏
 */

#define THROW_ERROR(code, message) \
    throw AppException(code, message)

#define THROW_ERROR_WITH_SEVERITY(code, message, severity) \
    throw AppException(code, message, severity)

#define TRY_CATCH(expr, error_code, error_message) \
    try { \
        expr; \
    } catch (const std::exception& e) { \
        getGlobalErrorHandler().handle( \
            AppException(error_code, error_message), \
            {{"original_error", e.what()}} \
        ); \
    }

#define TRY_CATCH_RETURN_DEFAULT(expr, default_value) \
    [&]() { \
        try { \
            return expr; \
        } catch (const std::exception& e) { \
            getGlobalErrorHandler().handle(e); \
            return default_value; \
        } \
    }()

// ============================================================================
// 预定义错误类型
// ============================================================================

namespace Errors {

// 客户端错误
inline AppException BadRequest(const std::string& msg) {
    return AppException(ErrorCode::BAD_REQUEST, msg);
}

inline AppException Unauthorized(const std::string& msg) {
    return AppException(ErrorCode::UNAUTHORIZED, msg);
}

inline AppException NotFound(const std::string& resource) {
    return AppException(ErrorCode::NOT_FOUND, resource + " not found");
}

inline AppException ValidationFailed(const std::string& field) {
    return AppException(ErrorCode::VALIDATION_ERROR, "Validation failed for: " + field);
}

// 服务器错误
inline AppException DatabaseError(const std::string& msg) {
    return AppException(ErrorCode::DATABASE_ERROR, msg, ErrorSeverity::CRITICAL);
}

inline AppException NetworkError(const std::string& msg) {
    return AppException(ErrorCode::NETWORK_ERROR, msg);
}

inline AppException Timeout(const std::string& operation) {
    return AppException(ErrorCode::TIMEOUT_ERROR, operation + " timed out");
}

inline AppException InternalError(const std::string& msg) {
    return AppException(ErrorCode::INTERNAL_ERROR, msg, ErrorSeverity::CRITICAL);
}

// 业务错误
inline AppException PaperNotFound(int paperId) {
    return AppException(ErrorCode::PAPER_NOT_FOUND,
                        "Paper not found: " + std::to_string(paperId));
}

inline AppException UserNotFound(int userId) {
    return AppException(ErrorCode::USER_NOT_FOUND,
                        "User not found: " + std::to_string(userId));
}

inline AppException CrawlFailed(const std::string& url) {
    return AppException(ErrorCode::CRAWL_FAILED, "Failed to crawl: " + url);
}

inline AppException DuplicatePaper(const std::string& title) {
    return AppException(ErrorCode::DUPLICATE_PAPER, "Duplicate paper: " + title);
}

// 安全错误
inline AppException SqlInjectionDetected(const std::string& input) {
    return AppException(ErrorCode::SQL_INJECTION_DETECTED,
                        "SQL injection detected: " + input,
                        ErrorSeverity::CRITICAL);
}

inline AppException AuthenticationFailed(const std::string& reason) {
    return AppException(ErrorCode::AUTHENTICATION_FAILED, reason);
}

} // namespace Errors

} // namespace PaperCrawler
