/**
 * Authentication Middleware
 *
 * Provides authentication and authorization checks for API requests
 * Validates JWT tokens and extracts user information
 */

#include "auth/AuthManager.hpp"
#include "auth/JwtUtils.hpp"
#include <map>
#include <string>

namespace PaperCrawler {

// ============================================================================
// Authentication Context
// ============================================================================

/**
 * @brief Authentication context for requests
 */
struct AuthContext {
    bool authenticated{false};
    int userId{0};
    std::string username;
    std::string email;
    std::string role;
    std::string error;

    AuthContext() = default;

    /**
     * @brief Check if user has specific role
     */
    bool hasRole(const std::string& requiredRole) const {
        if (!authenticated) return false;
        if (role == "admin") return true; // Admin has all permissions
        return role == requiredRole;
    }

    /**
     * @brief Check if user owns a resource
     */
    bool ownsResource(int resourceUserId) const {
        if (!authenticated) return false;
        if (role == "admin") return true; // Admin can access all
        return userId == resourceUserId;
    }
};

// ============================================================================
// Authentication Middleware Functions
// ============================================================================

/**
 * @brief Extract and verify JWT from Authorization header
 * @param authHeader Authorization header value
 * @return Authentication context
 *
 * Parses "Bearer <token>" format and verifies the JWT signature
 */
AuthContext authenticateRequest(const std::string& authHeader) {
    AuthContext ctx;

    // Check if Authorization header exists
    if (authHeader.empty()) {
        ctx.error = "Missing authorization header";
        return ctx;
    }

    // Extract Bearer token
    if (authHeader.substr(0, 7) != "Bearer ") {
        ctx.error = "Invalid authorization format (expected 'Bearer <token>')";
        return ctx;
    }

    std::string token = authHeader.substr(7);

    // Verify token with AuthManager
    auto& authManager = AuthManager::getInstance();
    auto userOpt = authManager.verifyToken(token);

    if (!userOpt.has_value()) {
        ctx.error = "Invalid or expired token";
        return ctx;
    }

    User user = userOpt.value();

    // Populate context
    ctx.authenticated = true;
    ctx.userId = user.id;
    ctx.username = user.username;
    ctx.email = user.email;
    ctx.role = user.role;

    return ctx;
}

/**
 * @brief Create authentication middleware for route protection
 * @return Middleware function
 *
 * Usage example:
 * @code
 * auto authMiddleware = createAuthMiddleware();
 *
 * // Apply to protected route
 * if (!authMiddleware(req, res)) {
 *     return; // Middleware already sent error response
 * }
 * @endcode
 */
std::function<bool(const std::string&, std::string&)> createAuthMiddleware() {
    return [](const std::string& authHeader, std::string& errorResponse) -> bool {
        AuthContext ctx = authenticateRequest(authHeader);

        if (!ctx.authenticated) {
            // Build error response
            errorResponse = "{\n";
            errorResponse += "  \"success\": false,\n";
            errorResponse += "  \"error\": \"UNAUTHORIZED\",\n";
            errorResponse += "  \"message\": \"" + ctx.error + "\"\n";
            errorResponse += "}";

            return false;
        }

        return true;
    };
}

/**
 * @brief Create role-based authorization middleware
 * @param requiredRole Required role (e.g., "admin", "premium")
 * @return Middleware function
 */
std::function<bool(const std::string&, std::string&)> createRoleMiddleware(
    const std::string& requiredRole) {

    return [requiredRole](const std::string& authHeader, std::string& errorResponse) -> bool {
        AuthContext ctx = authenticateRequest(authHeader);

        if (!ctx.authenticated) {
            errorResponse = "{\n";
            errorResponse += "  \"success\": false,\n";
            errorResponse += "  \"error\": \"UNAUTHORIZED\",\n";
            errorResponse += "  \"message\": \"Authentication required\"\n";
            errorResponse += "}";

            return false;
        }

        if (!ctx.hasRole(requiredRole)) {
            errorResponse = "{\n";
            errorResponse += "  \"success\": false,\n";
            errorResponse += "  \"error\": \"FORBIDDEN\",\n";
            errorResponse += "  \"message\": \"This action requires " + requiredRole + " role\"\n";
            errorResponse += "}";

            return false;
        }

        return true;
    };
}

/**
 * @brief Extract user ID from token
 * @param authHeader Authorization header
 * @return User ID or 0 if invalid
 */
int extractUserIdFromToken(const std::string& authHeader) {
    AuthContext ctx = authenticateRequest(authHeader);
    return ctx.userId;
}

/**
 * @brief Check if request is from admin
 * @param authHeader Authorization header
 * @return true if user is admin
 */
bool isAdminRequest(const std::string& authHeader) {
    AuthContext ctx = authenticateRequest(authHeader);
    return ctx.hasRole("admin");
}

} // namespace PaperCrawler
