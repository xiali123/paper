#pragma once

#include <string>
#include <stdexcept>
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <mutex>

namespace PaperCrawler {
namespace Core {

/**
 * @brief 错误代码枚举
 *
 * 通用错误代码定义，可扩展
 */
enum class ErrorCode {
    // 通用错误 (0-999)
    UNKNOWN = 0,             ///< 未知错误
    SUCCESS = 1,             ///< 成功

    // 客户端错误 (1000-1999)
    BAD_REQUEST = 1000,      ///< 错误的请求
    UNAUTHORIZED = 1001,     ///< 未授权
    FORBIDDEN = 1002,        ///< 禁止访问
    NOT_FOUND = 1003,        ///< 资源未找到
    CONFLICT = 1004,         ///< 冲突
    VALIDATION_ERROR = 1005, ///< 验证失败
    RATE_LIMIT_EXCEEDED = 1006, ///< 超过速率限制

    // 服务器错误 (2000-2999)
    INTERNAL_ERROR = 2000,   ///< 内部错误
    DATABASE_ERROR = 2001,   ///< 数据库错误
    NETWORK_ERROR = 2002,    ///< 网络错误
    TIMEOUT_ERROR = 2003,    ///< 超时错误
    SERVICE_UNAVAILABLE = 2004, ///< 服务不可用

    // 用户可扩展范围 (5000+)
    CUSTOM_ERROR = 5000      ///< 自定义错误起始码
};

/**
 * @brief 错误严重级别
 */
enum class ErrorSeverity {
    INFO,     ///< 信息性
    WARNING,  ///< 警告
    ERROR,    ///< 错误
    CRITICAL, ///< 严重
    FATAL     ///< 致命
};

/**
 * @brief 错误处理策略
 */
enum class ErrorHandlingStrategy {
    LOG_AND_CONTINUE,  ///< 记录日志并继续
    LOG_AND_THROW,     ///< 记录日志并抛出异常
    RETRY,             ///< 重试
    FALLBACK,          ///< 降级处理
    CIRCUIT_BREAKER    ///< 熔断
};

/**
 * @brief Exception - 通用异常类
 *
 * 提供了完整的异常处理功能，包括：
 * - 错误代码
 * - 错误消息
 * - 严重级别
 * - HTTP状态码映射
 * - 上下文信息
 *
 * @section example_usage 示例用法
 * @code
 * // 抛出异常
 * throw Exception(
 *     ErrorCode::DATABASE_ERROR,
 *     "Failed to connect to database",
 *     ErrorSeverity::CRITICAL
 * );
 *
 * // 捕获异常
 * try {
 *     // 业务逻辑
 * } catch (const Exception& e) {
 *     std::cerr << "Error: " << e.getMessage() << std::endl;
 *     std::cerr << "HTTP Status: " << e.getHttpStatusCode() << std::endl;
 * }
 * @endcode
 */
class Exception : public std::exception {
public:
    /**
     * @brief 构造函数
     *
     * @param code 错误代码
     * @param message 错误消息
     * @param severity 错误严重级别（默认ERROR）
     */
    Exception(
        ErrorCode code,
        const std::string& message,
        ErrorSeverity severity = ErrorSeverity::ERROR)
        : code_(code)
        , message_(message)
        , severity_(severity) {
    }

    /**
     * @brief 析构函数
     */
    virtual ~Exception() = default;

    /**
     * @brief 获取错误消息
     *
     * @return 错误消息
     */
    const char* what() const noexcept override {
        return message_.c_str();
    }

    /**
     * @brief 获取错误代码
     *
     * @return 错误代码
     */
    ErrorCode getCode() const {
        return code_;
    }

    /**
     * @brief 获取错误严重级别
     *
     * @return 错误严重级别
     */
    ErrorSeverity getSeverity() const {
        return severity_;
    }

    /**
     * @brief 获取错误消息
     *
     * @return 错误消息
     */
    const std::string& getMessage() const {
        return message_;
    }

    /**
     * @brief 获取HTTP状态码
     *
     * 将错误代码映射到HTTP状态码
     *
     * @return HTTP状态码
     */
    int getHttpStatusCode() const {
        switch (code_) {
            case ErrorCode::SUCCESS:
                return 200;
            case ErrorCode::BAD_REQUEST:
            case ErrorCode::VALIDATION_ERROR:
                return 400;
            case ErrorCode::UNAUTHORIZED:
                return 401;
            case ErrorCode::FORBIDDEN:
                return 403;
            case ErrorCode::NOT_FOUND:
                return 404;
            case ErrorCode::CONFLICT:
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

    /**
     * @brief 添加上下文信息
     *
     * @param key 上下文键
     * @param value 上下文值
     */
    void addContext(const std::string& key, const std::string& value) {
        context_[key] = value;
    }

    /**
     * @brief 获取上下文信息
     *
     * @return 上下文信息
     */
    const std::map<std::string, std::string>& getContext() const {
        return context_;
    }

private:
    ErrorCode code_;                                   ///< 错误代码
    std::string message_;                              ///< 错误消息
    ErrorSeverity severity_;                           ///< 错误严重级别
    std::map<std::string, std::string> context_;       ///< 上下文信息
};

/**
 * @brief IErrorHandler - 错误处理器接口
 *
 * 定义了错误处理器的接口契约
 */
class IErrorHandler {
public:
    virtual ~IErrorHandler() = default;

    /**
     * @brief 处理错误
     *
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
 * @brief ErrorHandler - 统一错误处理器
 *
 * 提供了完整的错误处理功能，包括：
 * - 错误处理器注册
 * - 错误处理策略
 * - 错误详情控制
 * - 错误恢复
 *
 * @section example_usage 示例用法
 * @code
 * auto& errorHandler = ErrorHandler::getInstance();
 *
 * // 注册自定义处理器
 * errorHandler.registerHandler(
 *     ErrorCode::DATABASE_ERROR,
 *     std::make_shared<DatabaseErrorHandler>()
 * );
 *
 * // 设置默认策略
 * errorHandler.setDefaultStrategy(ErrorHandlingStrategy::RETRY);
 *
 * // 处理错误
 * try {
 *     // 业务逻辑
 * } catch (const std::exception& e) {
 *     errorHandler.handle(e, {{"context", "value"}});
 * }
 * @endcode
 *
 * @threadsafe 所有公共方法都是线程安全的
 */
class ErrorHandler {
public:
    /**
     * @brief 获取单例实例
     *
     * @return ErrorHandler引用
     */
    static ErrorHandler& getInstance() {
        static ErrorHandler instance;
        return instance;
    }

    /**
     * @brief 处理异常
     *
     * @param error 异常对象
     * @param context 上下文信息（可选）
     *
     * @threadsafe 线程安全
     */
    void handle(
        const std::exception& error,
        const std::map<std::string, std::string>& context = {}
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        // 记录错误
        logError(error, context);

        // 查找注册的处理器
        // （简化实现，实际应该支持动态处理器注册）

        // 应用默认策略
        applyDefaultStrategy(error, context);
    }

    /**
     * @brief 注册错误处理器
     *
     * @param errorCode 错误代码
     * @param handler 处理器
     *
     * @threadsafe 线程安全
     */
    void registerHandler(ErrorCode errorCode, std::shared_ptr<IErrorHandler> handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handlers_[errorCode] = handler;
    }

    /**
     * @brief 设置默认处理策略
     *
     * @param strategy 处理策略
     *
     * @threadsafe 线程安全
     */
    void setDefaultStrategy(ErrorHandlingStrategy strategy) {
        std::lock_guard<std::mutex> lock(mutex_);
        defaultStrategy_ = strategy;
    }

    /**
     * @brief 启用/禁用错误详情
     *
     * @param enabled 是否启用
     *
     * @note 生产环境应禁用错误详情
     * @threadsafe 线程安全
     */
    void setDetailedErrors(bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        detailedErrors_ = enabled;
    }

private:
    /**
     * @brief 构造函数（私有）
     */
    ErrorHandler() = default;

    /**
     * @brief 析构函数（私有）
     */
    ~ErrorHandler() = default;

    // 禁止拷贝和移动
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;
    ErrorHandler(ErrorHandler&&) = delete;
    ErrorHandler& operator=(ErrorHandler&&) = delete;

    /**
     * @brief 记录错误
     */
    void logError(
        const std::exception& error,
        const std::map<std::string, std::string>& context
    ) {
        // TODO: 集成日志系统
    }

    /**
     * @brief 应用默认策略
     */
    void applyDefaultStrategy(
        const std::exception& error,
        const std::map<std::string, std::string>& context
    ) {
        switch (defaultStrategy_) {
            case ErrorHandlingStrategy::LOG_AND_CONTINUE:
                // 已记录，继续执行
                break;

            case ErrorHandlingStrategy::LOG_AND_THROW:
                // 已记录，重新抛出
                throw;

            case ErrorHandlingStrategy::RETRY:
                // TODO: 实现重试逻辑
                break;

            case ErrorHandlingStrategy::FALLBACK:
                // TODO: 实现降级逻辑
                break;

            case ErrorHandlingStrategy::CIRCUIT_BREAKER:
                // TODO: 实现熔断逻辑
                break;
        }
    }

    std::map<ErrorCode, std::shared_ptr<IErrorHandler>> handlers_;  ///< 错误处理器映射
    ErrorHandlingStrategy defaultStrategy_{ErrorHandlingStrategy::LOG_AND_THROW};  ///< 默认策略
    bool detailedErrors_{true};                        ///< 是否显示详细错误
    mutable std::mutex mutex_;                           ///< 互斥锁
};

/**
 * @brief 全局错误处理便捷函数
 */
class Errors {
public:
    // 客户端错误 (4xx)

    /**
     * @brief 创建错误请求异常
     */
    static Exception BadRequest(const std::string& msg) {
        return Exception(ErrorCode::BAD_REQUEST, msg);
    }

    /**
     * @brief 创建未授权异常
     */
    static Exception Unauthorized(const std::string& msg) {
        return Exception(ErrorCode::UNAUTHORIZED, msg);
    }

    /**
     * @brief 创建禁止访问异常
     */
    static Exception Forbidden(const std::string& msg) {
        return Exception(ErrorCode::FORBIDDEN, msg);
    }

    /**
     * @brief 创建资源未找到异常
     */
    static Exception NotFound(const std::string& resource) {
        return Exception(ErrorCode::NOT_FOUND, resource + " not found");
    }

    /**
     * @brief 创建验证失败异常
     */
    static Exception ValidationFailed(const std::string& field) {
        return Exception(ErrorCode::VALIDATION_ERROR, "Validation failed for: " + field);
    }

    // 服务器错误 (5xx)

    /**
     * @brief 创建数据库错误异常
     */
    static Exception DatabaseError(const std::string& msg) {
        return Exception(ErrorCode::DATABASE_ERROR, msg, ErrorSeverity::CRITICAL);
    }

    /**
     * @brief 创建网络错误异常
     */
    static Exception NetworkError(const std::string& msg) {
        return Exception(ErrorCode::NETWORK_ERROR, msg);
    }

    /**
     * @brief 创建超时异常
     */
    static Exception Timeout(const std::string& operation) {
        return Exception(ErrorCode::TIMEOUT_ERROR, operation + " timed out");
    }

    /**
     * @brief 创建内部错误异常
     */
    static Exception InternalError(const std::string& msg) {
        return Exception(ErrorCode::INTERNAL_ERROR, msg, ErrorSeverity::CRITICAL);
    }

    /**
     * @brief 创建服务不可用异常
     */
    static Exception ServiceUnavailable(const std::string& msg) {
        return Exception(ErrorCode::SERVICE_UNAVAILABLE, msg);
    }
};

} // namespace Core
} // namespace PaperCrawler

// ============================================================================
// 便捷宏
// ============================================================================

/**
 * @brief 抛出异常宏
 */
#define THROW_ERROR(code, message) \
    throw PaperCrawler::Core::Exception(code, message)

#define THROW_ERROR_WITH_SEVERITY(code, message, severity) \
    throw PaperCrawler::Core::Exception(code, message, severity)

/**
 * @brief TRY-CATCH宏
 */
#define TRY_CATCH(expr, error_code, error_message) \
    try { \
        expr; \
    } catch (const std::exception& e) { \
        PaperCrawler::Core::ErrorHandler::getInstance().handle( \
            PaperCrawler::Core::Exception(error_code, error_message), \
            {{"original_error", e.what()}} \
        ); \
    }

#define TRY_CATCH_RETURN_DEFAULT(expr, default_value) \
    [&]() { \
        try { \
            return expr; \
        } catch (const std::exception& e) { \
            PaperCrawler::Core::ErrorHandler::getInstance().handle(e); \
            return default_value; \
        } \
    }()
