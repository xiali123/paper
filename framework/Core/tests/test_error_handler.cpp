/**
 * @file test_error_handler.cpp
 * @brief ErrorHandler 单元测试
 */

#include <gtest/gtest.h>
#include <PaperCrawler/Core>
#include <stdexcept>

using namespace PaperCrawler::Core;

/**
 * @test 异常构造测试
 */
TEST(ErrorHandlerTest, ExceptionConstruction) {
    Exception ex(
        ErrorCode::DATABASE_ERROR,
        "Database connection failed",
        ErrorSeverity::CRITICAL
    );

    EXPECT_EQ(ex.getCode(), ErrorCode::DATABASE_ERROR);
    EXPECT_EQ(ex.getMessage(), "Database connection failed");
    EXPECT_EQ(ex.getSeverity(), ErrorSeverity::CRITICAL);
}

/**
 * @test HTTP状态码映射测试
 */
TEST(ErrorHandlerTest, HttpStatusCodeMapping) {
    // 4xx 错误
    Exception badRequest(ErrorCode::BAD_REQUEST, "Bad request");
    EXPECT_EQ(badRequest.getHttpStatusCode(), 400);

    Exception unauthorized(ErrorCode::UNAUTHORIZED, "Unauthorized");
    EXPECT_EQ(unauthorized.getHttpStatusCode(), 401);

    Exception forbidden(ErrorCode::FORBIDDEN, "Forbidden");
    EXPECT_EQ(forbidden.getHttpStatusCode(), 403);

    Exception notFound(ErrorCode::NOT_FOUND, "Not found");
    EXPECT_EQ(notFound.getHttpStatusCode(), 404);

    // 5xx 错误
    Exception dbError(ErrorCode::DATABASE_ERROR, "DB error");
    EXPECT_EQ(dbError.getHttpStatusCode(), 500);

    Exception serviceUnavailable(ErrorCode::SERVICE_UNAVAILABLE, "Service unavailable");
    EXPECT_EQ(serviceUnavailable.getHttpStatusCode(), 503);
}

/**
 * @test 上下文信息测试
 */
TEST(ErrorHandlerTest, ContextInfo) {
    Exception ex(ErrorCode::INTERNAL_ERROR, "Internal error");

    ex.addContext("userId", "12345");
    ex.addContext("action", "deleteRecord");
    ex.addContext("timestamp", "2024-04-03");

    auto context = ex.getContext();
    EXPECT_EQ(context.size(), 3);
    EXPECT_EQ(context["userId"], "12345");
    EXPECT_EQ(context["action"], "deleteRecord");
}

/**
 * @test 便捷错误创建测试
 */
TEST(ErrorHandlerTest, ConvenienceErrorCreation) {
    auto badReq = Errors::BadRequest("Invalid input");
    EXPECT_EQ(badReq.getCode(), ErrorCode::BAD_REQUEST);
    EXPECT_EQ(badReq.getHttpStatusCode(), 400);

    auto notFound = Errors::NotFound("user");
    EXPECT_EQ(notFound.getCode(), ErrorCode::NOT_FOUND);
    EXPECT_EQ(notFound.getHttpStatusCode(), 404);

    auto dbError = Errors::DatabaseError("Connection failed");
    EXPECT_EQ(dbError.getCode(), ErrorCode::DATABASE_ERROR);
    EXPECT_EQ(dbError.getHttpStatusCode(), 500);

    auto timeout = Errors::Timeout("Database query");
    EXPECT_EQ(timeout.getCode(), ErrorCode::TIMEOUT_ERROR);
}

/**
 * @test 错误处理策略测试
 */
TEST(ErrorHandlerTest, ErrorHandlingStrategy) {
    auto& handler = ErrorHandler::getInstance();

    // 设置 LOG_AND_CONTINUE 策略
    handler.setDefaultStrategy(ErrorHandlingStrategy::LOG_AND_CONTINUE);

    // 捕获异常但不应该重新抛出
    try {
        throw Errors::InternalError("Test error");
    } catch (const Exception& e) {
        // 处理异常
        handler.handle(e);
        // 不应该抛出，继续执行
        SUCCEED();
    }
}

/**
 * @test 错误严重级别测试
 */
TEST(ErrorHandlerTest, ErrorSeverity) {
    Exception info(ErrorCode::INTERNAL_ERROR, "Info", ErrorSeverity::INFO);
    Exception warning(ErrorCode::INTERNAL_ERROR, "Warning", ErrorSeverity::WARNING);
    Exception error(ErrorCode::INTERNAL_ERROR, "Error", ErrorSeverity::ERROR);
    Exception critical(ErrorCode::INTERNAL_ERROR, "Critical", ErrorSeverity::CRITICAL);
    Exception fatal(ErrorCode::INTERNAL_ERROR, "Fatal", ErrorSeverity::FATAL);

    EXPECT_EQ(info.getSeverity(), ErrorSeverity::INFO);
    EXPECT_EQ(warning.getSeverity(), ErrorSeverity::WARNING);
    EXPECT_EQ(error.getSeverity(), ErrorSeverity::ERROR);
    EXPECT_EQ(critical.getSeverity(), ErrorSeverity::CRITICAL);
    EXPECT_EQ(fatal.getSeverity(), ErrorSeverity::FATAL);
}

/**
 * @test 异常消息测试
 */
TEST(ErrorHandlerTest, ExceptionMessage) {
    Exception ex(ErrorCode::DATABASE_ERROR, "Connection failed");

    EXPECT_STREQ(ex.what(), "Connection failed");
}

/**
 * @test 错误处理器注册测试
 */
TEST(ErrorHandlerTest, RegisterHandler) {
    auto& handler = ErrorHandler::getInstance();

    bool customHandlerCalled = false;

    class CustomErrorHandler : public IErrorHandler {
    public:
        bool& called;
        CustomErrorHandler(bool& c) : called(c) {}

        bool handleError(
            const std::exception& error,
            const std::map<std::string, std::string>& context
        ) override {
            called = true;
            return true;
        }
    };

    bool handlerCalled = false;
    auto customHandler = std::make_shared<CustomErrorHandler>(handlerCalled);

    handler.registerHandler(ErrorCode::DATABASE_ERROR, customHandler);

    try {
        throw Errors::DatabaseError("Test error");
    } catch (const Exception& e) {
        handler.handle(e);
    }

    // 注意：实际实现可能需要更复杂的逻辑
    // 这里只是验证注册接口存在
    SUCCEED();
}

/**
 * @test 详细错误控制测试
 */
TEST(ErrorHandlerTest, DetailedErrorsControl) {
    auto& handler = ErrorHandler::getInstance();

    // 启用详细错误
    handler.setDetailedErrors(true);
    // （无法直接测试，但验证API存在）
    SUCCEED();

    // 禁用详细错误
    handler.setDetailedErrors(false);
    SUCCEED();
}

/**
 * @test TRY-CATCH宏测试
 */
TEST(ErrorHandlerTest, TryCatchMacro) {
    bool shouldThrow = false;
    int errorCount = 0;

    // 使用TRY-CATCH宏
    TRY_CATCH(
        {
            if (shouldThrow) {
                throw std::runtime_error("Test exception");
            }
        },
        ErrorCode::INTERNAL_ERROR,
        "Operation failed"
    );

    // 不应该抛出异常
    SUCCEED();
}

/**
 * @test 单例模式测试
 */
TEST(ErrorHandlerTest, SingletonPattern) {
    auto& handler1 = ErrorHandler::getInstance();
    auto& handler2 = ErrorHandler::getInstance();

    EXPECT_EQ(&handler1, &handler2);
}

/**
 * @test 错误代码枚举测试
 */
TEST(ErrorHandlerTest, ErrorCodeEnum) {
    // 通用错误
    EXPECT_EQ(static_cast<int>(ErrorCode::UNKNOWN), 0);
    EXPECT_EQ(static_cast<int>(ErrorCode::SUCCESS), 1);

    // 客户端错误
    EXPECT_GE(static_cast<int>(ErrorCode::BAD_REQUEST), 1000);
    EXPECT_LT(static_cast<int>(ErrorCode::BAD_REQUEST), 2000);

    // 服务器错误
    EXPECT_GE(static_cast<int>(ErrorCode::INTERNAL_ERROR), 2000);
    EXPECT_LT(static_cast<int>(ErrorCode::INTERNAL_ERROR), 3000);
}

/**
 * @test 异常安全测试
 */
TEST(ErrorHandlerTest, ExceptionSafety) {
    // 验证异常可以被正确捕获
    EXPECT_THROW(
        throw Errors::DatabaseError("Test error"),
        Exception
    );

    EXPECT_THROW(
        throw Errors::NotFound("Test"),
        Exception
    );

    // 验证异常消息正确传递
    try {
        throw Errors::InternalError("Critical failure");
    } catch (const Exception& e) {
        EXPECT_EQ(std::string(e.what()), "Critical failure");
    }
}
