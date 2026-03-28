/**
 * API Server Authentication Integration
 *
 * This file shows how to integrate authentication into the existing api_server.cpp
 * Add these sections to your api_server.cpp file
 */

#include "auth/AuthManager.hpp"
#include "auth_middleware.cpp"
#include "auth_handlers.cpp"

namespace PaperCrawler {

// ============================================================================
// Step 1: Initialize AuthManager in server startup
// ============================================================================

void initializeAuthentication() {
    auto& authManager = AuthManager::getInstance();

    // Configure with your database manager
    authManager.initialize(&databaseManager); // or &sqliteManager

    // Set JWT secret (load from config file in production)
    authManager.setJwtSecret("your-secret-key-change-in-production");

    // Set token expiry times
    authManager.setTokenExpiry(15, 30); // 15 min access, 30 days refresh

    // Set rate limiting
    authManager.setRateLimitingParams(5, 60000, 30); // 5 attempts, 1 min, 30 min lock

    authManager.setEnabled(true);

    std::cout << "Authentication system initialized" << std::endl;
}

// ============================================================================
// Step 2: Add authentication routes to request router
// ============================================================================

// In your routeRequest function, add these cases BEFORE existing routes:

std::string routeRequest(const RequestInfo& info) {
    // ========================================================================
    // Authentication routes (no auth required)
    // ========================================================================

    if (info.path == "/api/auth/register" && info.method == "POST") {
        std::string ipAddress = extractIpAddress(info.clientSocket);
        return handleRegister(info.body, ipAddress);
    }

    if (info.path == "/api/auth/login" && info.method == "POST") {
        std::string ipAddress = extractIpAddress(info.clientSocket);
        std::string userAgent = extractUserAgent(info);
        return handleLogin(info.body, ipAddress, userAgent);
    }

    if (info.path == "/api/auth/logout" && info.method == "POST") {
        return handleLogout(info.body);
    }

    if (info.path == "/api/auth/refresh" && info.method == "POST") {
        return handleRefreshToken(info.body);
    }

    // ========================================================================
    // Authenticated routes (require valid JWT)
    // ========================================================================

    if (info.path == "/api/auth/me" && info.method == "GET") {
        std::string authHeader = extractAuthHeader(info);
        std::string errorResponse;

        // Check authentication
        auto authMiddleware = createAuthMiddleware();
        if (!authMiddleware(authHeader, errorResponse)) {
            return errorResponse; // Return 401 error
        }

        return handleGetCurrentUser(authHeader);
    }

    // ========================================================================
    // Existing protected routes (add authentication check)
    // ========================================================================

    if (info.path == "/api/search" && info.method == "GET") {
        std::string authHeader = extractAuthHeader(info);
        std::string errorResponse;

        // Require authentication
        auto authMiddleware = createAuthMiddleware();
        if (!authMiddleware(authHeader, errorResponse)) {
            return errorResponse;
        }

        // Get user context
        int userId = extractUserIdFromToken(authHeader);

        // Add user context to search
        return handleSearch(info.query, userId);
    }

    if (info.path == "/api/papers" && info.method == "GET") {
        std::string authHeader = extractAuthHeader(info);
        std::string errorResponse;

        // Require authentication
        auto authMiddleware = createAuthMiddleware();
        if (!authMiddleware(authHeader, errorResponse)) {
            return errorResponse;
        }

        return handleGetPapers(info.query);
    }

    if (info.path.find("/api/papers/") == 0 && info.method == "GET") {
        std::string authHeader = extractAuthHeader(info);
        std::string errorResponse;

        // Require authentication
        auto authMiddleware = createAuthMiddleware();
        if (!authMiddleware(authHeader, errorResponse)) {
            return errorResponse;
        }

        // Extract paper ID from path
        std::string paperId = info.path.substr(12); // /api/papers/{id}

        return handleGetPaper(paperId);
    }

    if (info.path == "/api/stats/overview" && info.method == "GET") {
        std::string authHeader = extractAuthHeader(info);
        std::string errorResponse;

        // Require authentication
        auto authMiddleware = createAuthMiddleware();
        if (!authMiddleware(authHeader, errorResponse)) {
            return errorResponse;
        }

        return handleStatsOverview();
    }

    // ========================================================================
    // Admin-only routes
    // ========================================================================

    if (info.path == "/api/admin/users" && info.method == "GET") {
        std::string authHeader = extractAuthHeader(info);
        std::string errorResponse;

        // Require admin role
        auto adminMiddleware = createRoleMiddleware("admin");
        if (!adminMiddleware(authHeader, errorResponse)) {
            return errorResponse;
        }

        return handleGetAllUsers();
    }

    // ========================================================================
    // Public routes (no authentication required)
    // ========================================================================

    if (info.path == "/health" && info.method == "GET") {
        return handleHealth();
    }

    // ========================================================================
    // 404 - Not Found
    // ========================================================================

    return buildErrorResponse(404, "NOT_FOUND", "Endpoint not found");
}

// ============================================================================
// Step 3: Add helper functions for header extraction
// ============================================================================

/**
 * @brief Extract Authorization header from request
 */
std::string extractAuthHeader(const RequestInfo& info) {
    // Implementation depends on your HTTP library
    // Example for common libraries:

    // For raw HTTP:
    auto it = info.headers.find("Authorization");
    if (it != info.headers.end()) {
        return it->second;
    }

    return "";
}

/**
 * @brief Extract IP address from client socket
 */
std::string extractIpAddress(SOCKET clientSocket) {
    // Implementation depends on your socket library

    // For Berkeley sockets:
    struct sockaddr_in addr;
    socklen_t addrSize = sizeof(addr);
    getpeername(clientSocket, (struct sockaddr*)&addr, &addrSize);

    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ipStr, INET_ADDRSTRLEN);

    return std::string(ipStr);
}

/**
 * @brief Extract User-Agent from request
 */
std::string extractUserAgent(const RequestInfo& info) {
    auto it = info.headers.find("User-Agent");
    if (it != info.headers.end()) {
        return it->second;
    }

    return "Unknown";
}

// ============================================================================
// Step 4: Update existing handlers to accept user context
// ============================================================================

/**
 * Example: Update search handler to include user-specific data
 */
std::string handleSearch(const std::string& query, int userId) {
    auto& authManager = AuthManager::getInstance();

    // Get user info for personalization
    auto userOpt = authManager.getUserById(userId);
    if (userOpt.has_value()) {
        // Add user context to search
        // e.g., filter by user's bookmarks, reading history, etc.
    }

    // ... existing search logic ...

    return "{}"; // Your search results
}

} // namespace PaperCrawler

// ============================================================================
// Migration Checklist
// ============================================================================
/*
 * To integrate authentication into your existing api_server.cpp:
 *
 * 1. Add includes:
 *    - #include "auth/AuthManager.hpp"
 *    - #include "auth_middleware.cpp"
 *    - #include "auth_handlers.cpp"
 *
 * 2. In server initialization:
 *    - Call initializeAuthentication()
 *    - Configure JWT secret from config file
 *
 * 3. In request router:
 *    - Add auth routes (register, login, logout, refresh)
 *    - Add authentication middleware to protected routes
 *    - Add role-based middleware to admin routes
 *
 * 4. Update existing handlers:
 *    - Add userId parameter where needed
 *    - Include user-specific data in responses
 *
 * 5. Test the integration:
 *    - Register new user
 *    - Login and receive tokens
 *    - Access protected routes with tokens
 *    - Verify token refresh works
 */
