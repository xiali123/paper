/**
 * Authentication API Handlers
 *
 * HTTP request handlers for authentication endpoints
 * Integrates with AuthManager for business logic
 *
 * Endpoints:
 * - POST /api/auth/register - User registration
 * - POST /api/auth/login - User login
 * - POST /api/auth/logout - User logout
 * - POST /api/auth/refresh - Token refresh
 * - GET /api/auth/me - Get current user
 */

#include "auth/AuthManager.hpp"
#include "auth/JwtUtils.hpp"
#include <sstream>
#include <iomanip>
#include <regex>

namespace PaperCrawler {

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Escape JSON string
 */
std::string escapeJsonString(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 1.2);

    for (char c : str) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (c < 0x20) {
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    escaped += oss.str();
                } else {
                    escaped += c;
                }
        }
    }

    return escaped;
}

/**
 * @brief Build success response
 */
std::string buildSuccessResponse(const std::string& data) {
    std::ostringstream json;
    int64_t timestamp = std::chrono::system_clock::now().time_since_epoch() /
                        std::chrono::seconds(1);

    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"data\": " << data << ",\n";
    json << "  \"timestamp\": " << timestamp << "\n";
    json << "}";

    return json.str();
}

/**
 * @brief Build error response
 */
std::string buildErrorResponse(int status, const std::string& code, const std::string& message) {
    std::ostringstream json;
    int64_t timestamp = std::chrono::system_clock::now().time_since_epoch() /
                        std::chrono::seconds(1);

    json << "{\n";
    json << "  \"success\": false,\n";
    json << "  \"error\": \"" << escapeJsonString(code) << "\",\n";
    json << "  \"message\": \"" << escapeJsonString(message) << "\",\n";
    json << "  \"timestamp\": " << timestamp << "\n";
    json << "}";

    return json.str();
}

/**
 * @brief Parse JSON key-value
 */
std::string parseJsonKeyValue(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]+)\"";
    std::regex regex(pattern);
    std::smatch match;

    if (std::regex_search(json, match, regex) && match.size() > 1) {
        return match[1].str();
    }

    return "";
}

// ============================================================================
// Authentication Endpoint Handlers
// ============================================================================

/**
 * @brief POST /api/auth/register
 * Register new user account
 */
std::string handleRegister(const std::string& body, const std::string& ipAddress) {
    auto& authManager = AuthManager::getInstance();

    try {
        // Parse JSON body (simplified parsing)
        RegistrationData data;
        data.username = parseJsonKeyValue(body, "username");
        data.email = parseJsonKeyValue(body, "email");
        data.password = parseJsonKeyValue(body, "password");
        data.fullName = parseJsonKeyValue(body, "fullName");
        data.affiliation = parseJsonKeyValue(body, "affiliation");

        // Validate input
        if (data.username.empty() || data.email.empty() || data.password.empty()) {
            return buildErrorResponse(400, "INVALID_INPUT",
                                    "Username, email, and password are required");
        }

        // Register user
        AuthResult result = authManager.registerUser(data);

        if (!result.success) {
            return buildErrorResponse(400, result.errorCode, result.errorMessage);
        }

        // Build response with tokens
        std::ostringstream json;
        json << "{\n";
        json << "  \"user\": {\n";
        json << "    \"id\": " << result.user.id << ",\n";
        json << "    \"username\": \"" << escapeJsonString(result.user.username) << "\",\n";
        json << "    \"email\": \"" << escapeJsonString(result.user.email) << "\",\n";
        json << "    \"fullName\": \"" << escapeJsonString(result.user.fullName) << "\",\n";
        json << "    \"role\": \"" << result.user.role << "\",\n";
        json << "    \"isActive\": " << (result.user.isActive ? "true" : "false") << ",\n";
        json << "    \"isVerified\": " << (result.user.isVerified ? "true" : "false") << "\n";
        json << "  },\n";
        json << "  \"tokens\": {\n";
        json << "    \"accessToken\": \"" << result.tokens.accessToken << "\",\n";
        json << "    \"refreshToken\": \"" << result.tokens.refreshToken << "\",\n";
        json << "    \"expiresAt\": " << result.tokens.expiresAt << "\n";
        json << "  }\n";
        json << "}";

        return buildSuccessResponse(json.str());

    } catch (const std::exception& e) {
        return buildErrorResponse(500, "REGISTRATION_ERROR", e.what());
    }
}

/**
 * @brief POST /api/auth/login
 * Authenticate user and return tokens
 */
std::string handleLogin(const std::string& body, const std::string& ipAddress,
                       const std::string& userAgent) {
    auto& authManager = AuthManager::getInstance();

    try {
        // Parse credentials
        LoginCredentials creds;
        creds.email = parseJsonKeyValue(body, "email");
        creds.password = parseJsonKeyValue(body, "password");
        creds.ipAddress = ipAddress;
        creds.userAgent = userAgent;

        // Validate input
        if (creds.email.empty() || creds.password.empty()) {
            return buildErrorResponse(400, "INVALID_INPUT",
                                    "Email and password are required");
        }

        // Authenticate
        AuthResult result = authManager.login(creds);

        if (!result.success) {
            return buildErrorResponse(401, result.errorCode, result.errorMessage);
        }

        // Build response
        std::ostringstream json;
        json << "{\n";
        json << "  \"user\": {\n";
        json << "    \"id\": " << result.user.id << ",\n";
        json << "    \"username\": \"" << escapeJsonString(result.user.username) << "\",\n";
        json << "    \"email\": \"" << escapeJsonString(result.user.email) << "\",\n";
        json << "    \"fullName\": \"" << escapeJsonString(result.user.fullName) << "\",\n";
        json << "    \"role\": \"" << result.user.role << "\"\n";
        json << "  },\n";
        json << "  \"tokens\": {\n";
        json << "    \"accessToken\": \"" << result.tokens.accessToken << "\",\n";
        json << "    \"refreshToken\": \"" << result.tokens.refreshToken << "\",\n";
        json << "    \"expiresAt\": " << result.tokens.expiresAt << "\n";
        json << "  }\n";
        json << "}";

        return buildSuccessResponse(json.str());

    } catch (const std::exception& e) {
        return buildErrorResponse(500, "LOGIN_ERROR", e.what());
    }
}

/**
 * @brief POST /api/auth/logout
 * Invalidate refresh token
 */
std::string handleLogout(const std::string& body) {
    auto& authManager = AuthManager::getInstance();

    try {
        std::string refreshToken = parseJsonKeyValue(body, "refreshToken");

        if (refreshToken.empty()) {
            return buildErrorResponse(400, "INVALID_INPUT",
                                    "Refresh token is required");
        }

        bool success = authManager.logout(refreshToken);

        if (!success) {
            return buildErrorResponse(400, "LOGOUT_FAILED",
                                    "Invalid or expired refresh token");
        }

        std::ostringstream json;
        json << "{\n";
        json << "  \"message\": \"Logged out successfully\"\n";
        json << "}";

        return buildSuccessResponse(json.str());

    } catch (const std::exception& e) {
        return buildErrorResponse(500, "LOGOUT_ERROR", e.what());
    }
}

/**
 * @brief POST /api/auth/refresh
 * Refresh access token
 */
std::string handleRefreshToken(const std::string& body) {
    auto& authManager = AuthManager::getInstance();

    try {
        std::string refreshToken = parseJsonKeyValue(body, "refreshToken");

        if (refreshToken.empty()) {
            return buildErrorResponse(400, "INVALID_INPUT",
                                    "Refresh token is required");
        }

        std::string newAccessToken = authManager.refreshAccessToken(refreshToken);

        if (newAccessToken.empty()) {
            return buildErrorResponse(401, "INVALID_TOKEN",
                                    "Invalid or expired refresh token");
        }

        std::ostringstream json;
        json << "{\n";
        json << "  \"accessToken\": \"" << newAccessToken << "\",\n";
        json << "  \"expiresAt\": " << (std::chrono::system_clock::now().time_since_epoch() /
                                       std::chrono::seconds(1) + 900) << "\n"; // 15 min
        json << "}";

        return buildSuccessResponse(json.str());

    } catch (const std::exception& e) {
        return buildErrorResponse(500, "REFRESH_ERROR", e.what());
    }
}

/**
 * @brief GET /api/auth/me
 * Get current user info
 */
std::string handleGetCurrentUser(const std::string& authHeader) {
    auto& authManager = AuthManager::getInstance();

    try {
        // Extract token from Authorization header
        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            return buildErrorResponse(401, "UNAUTHORIZED",
                                    "Missing or invalid authorization header");
        }

        std::string accessToken = authHeader.substr(7);

        // Verify token and get user
        std::optional<User> userOpt = authManager.verifyToken(accessToken);

        if (!userOpt.has_value()) {
            return buildErrorResponse(401, "INVALID_TOKEN",
                                    "Invalid or expired access token");
        }

        User user = userOpt.value();

        // Build response
        std::ostringstream json;
        json << "{\n";
        json << "  \"id\": " << user.id << ",\n";
        json << "  \"username\": \"" << escapeJsonString(user.username) << "\",\n";
        json << "  \"email\": \"" << escapeJsonString(user.email) << "\",\n";
        json << "  \"fullName\": \"" << escapeJsonString(user.fullName) << "\",\n";
        json << "  \"role\": \"" << user.role << "\",\n";
        json << "  \"isActive\": " << (user.isActive ? "true" : "false") << ",\n";
        json << "  \"isVerified\": " << (user.isVerified ? "true" : "false") << ",\n";
        json << "  \"lastLoginAt\": " << user.lastLoginAt << ",\n";
        json << "  \"createdAt\": " << user.createdAt << "\n";
        json << "}";

        return buildSuccessResponse(json.str());

    } catch (const std::exception& e) {
        return buildErrorResponse(500, "USER_INFO_ERROR", e.what());
    }
}

} // namespace PaperCrawler
