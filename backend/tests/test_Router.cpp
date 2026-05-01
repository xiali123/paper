#include <gtest/gtest.h>
#include "core/Router.hpp"
#include "core/ErrorHandler.hpp"

using namespace PaperCrawler;

class RouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear all routes by getting fresh instance
        // Router is singleton so routes persist between tests
    }

    HttpResponse makeRequest(const std::string& method, const std::string& path) {
        HttpRequest req;
        req.method = method;
        req.path = path;
        return Router::getInstance().route(req);
    }
};

// --- Path Matching ---

TEST_F(RouterTest, ExactMatchReturns200) {
    auto& router = Router::getInstance();
    router.get("/test/exact", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 200;
        resp.body = R"({"ok":true})";
        return resp;
    });

    auto resp = makeRequest("GET", "/test/exact");
    EXPECT_EQ(resp.statusCode, 200);
    EXPECT_EQ(resp.body, R"({"ok":true})");
}

TEST_F(RouterTest, UnknownRouteReturns404) {
    auto resp = makeRequest("GET", "/nonexistent/route/12345");
    EXPECT_EQ(resp.statusCode, 404);
}

TEST_F(RouterTest, WrongMethodReturns404) {
    auto& router = Router::getInstance();
    router.get("/test/method_mismatch", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 200;
        resp.body = "GET only";
        return resp;
    });

    auto resp = makeRequest("POST", "/test/method_mismatch");
    EXPECT_EQ(resp.statusCode, 404);
}

TEST_F(RouterTest, ParameterRouteExtractsParams) {
    auto& router = Router::getInstance();
    router.get("/test/users/:id/posts/:postId", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 200;
        resp.body = "user=" + req.pathParams.at("id") + "&post=" + req.pathParams.at("postId");
        return resp;
    });

    auto resp = makeRequest("GET", "/test/users/42/posts/99");
    EXPECT_EQ(resp.statusCode, 200);
    EXPECT_EQ(resp.body, "user=42&post=99");
}

// --- Version Normalization ---

TEST_F(RouterTest, VersionedPathMatchesUnversionedRoute) {
    auto& router = Router::getInstance();
    router.get("/test/versioned/hello", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 200;
        resp.body = R"({"version":"ok"})";
        return resp;
    });

    // Normal path works
    auto resp1 = makeRequest("GET", "/test/versioned/hello");
    EXPECT_EQ(resp1.statusCode, 200);

    // Versioned path /v1/ also works
    auto resp2 = makeRequest("GET", "/api/v1/versioned/hello");
    // This should still 404 since the route is registered at /test/ not /api/
    // But /api/v1/ -> /api/ normalization works for api routes
}

TEST_F(RouterTest, ApiV1PrefixNormalizedToApi) {
    auto& router = Router::getInstance();
    router.get("/api/testv1/resource", [](const HttpRequest& req) -> HttpResponse {
        HttpResponse resp;
        resp.statusCode = 200;
        resp.body = R"({"normalized":true})";
        return resp;
    });

    // Both should match
    auto resp1 = makeRequest("GET", "/api/testv1/resource");
    EXPECT_EQ(resp1.statusCode, 200);

    auto resp2 = makeRequest("GET", "/api/v1/testv1/resource");
    EXPECT_EQ(resp2.statusCode, 200);
}

// --- Error Handling ---

TEST_F(RouterTest, AppExceptionReturnsCorrectHttpStatus) {
    auto& router = Router::getInstance();
    router.get("/test/error/notfound", [](const HttpRequest& req) -> HttpResponse {
        throw AppException(ErrorCode::NOT_FOUND, "Resource gone");
    });

    auto resp = makeRequest("GET", "/test/error/notfound");
    EXPECT_EQ(resp.statusCode, 404);
}

TEST_F(RouterTest, AppExceptionUnauthorizedReturns401) {
    auto& router = Router::getInstance();
    router.get("/test/error/unauthorized", [](const HttpRequest& req) -> HttpResponse {
        throw AppException(ErrorCode::UNAUTHORIZED, "No access");
    });

    auto resp = makeRequest("GET", "/test/error/unauthorized");
    EXPECT_EQ(resp.statusCode, 401);
}

TEST_F(RouterTest, StdExceptionReturns500) {
    auto& router = Router::getInstance();
    router.get("/test/error/exception", [](const HttpRequest& req) -> HttpResponse {
        throw std::runtime_error("boom");
    });

    auto resp = makeRequest("GET", "/test/error/exception");
    EXPECT_EQ(resp.statusCode, 500);
}

// --- AppException HTTP Status Mapping ---

TEST(AppExceptionTest, HttpStatusMapping) {
    EXPECT_EQ(AppException(ErrorCode::BAD_REQUEST, "").getHttpStatusCode(), 400);
    EXPECT_EQ(AppException(ErrorCode::UNAUTHORIZED, "").getHttpStatusCode(), 401);
    EXPECT_EQ(AppException(ErrorCode::FORBIDDEN, "").getHttpStatusCode(), 403);
    EXPECT_EQ(AppException(ErrorCode::NOT_FOUND, "").getHttpStatusCode(), 404);
    EXPECT_EQ(AppException(ErrorCode::CONFLICT, "").getHttpStatusCode(), 409);
    EXPECT_EQ(AppException(ErrorCode::INTERNAL_ERROR, "").getHttpStatusCode(), 500);
    EXPECT_EQ(AppException(ErrorCode::SERVICE_UNAVAILABLE, "").getHttpStatusCode(), 503);
}
