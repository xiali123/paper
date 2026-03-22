#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include "database/DatabaseManager.hpp"
#include "database/SqliteManager.hpp"

namespace PaperCrawler {

// ============================================================================
// Forward Declarations
// ============================================================================

class PasswordHasher;
class RateLimiter;
class JwtUtils;

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief User account information
 */
struct User {
    int id;
    std::string username;
    std::string email;
    std::string fullName;
    std::string role;
    bool isActive;
    bool isVerified;
    int64_t lastLoginAt;
    int64_t createdAt;
    int64_t updatedAt;

    User() : id(0), isActive(false), isVerified(false), lastLoginAt(0), createdAt(0), updatedAt(0) {}
};

/**
 * @brief Authentication tokens
 */
struct AuthTokens {
    std::string accessToken;   // JWT short-lived (15 min)
    std::string refreshToken;  // Long-lived token (30 days)
    int64_t expiresAt;         // Access token expiry (Unix timestamp)

    AuthTokens() : expiresAt(0) {}
};

/**
 * @brief Login credentials
 */
struct LoginCredentials {
    std::string email;
    std::string password;
    std::string ipAddress;
    std::string userAgent;
};

/**
 * @brief Registration data
 */
struct RegistrationData {
    std::string username;
    std::string email;
    std::string password;
    std::string fullName;
    std::string affiliation;
};

/**
 * @brief Authentication result
 */
struct AuthResult {
    bool success;
    User user;
    AuthTokens tokens;
    std::string errorCode;
    std::string errorMessage;

    AuthResult() : success(false) {}
};

/**
 * @brief JWT payload
 */
struct JwtPayload {
    int userId;
    std::string username;
    std::string email;
    std::string role;
    int64_t issuedAt;
    int64_t expiresAt;

    JwtPayload() : userId(0), issuedAt(0), expiresAt(0) {}

    /**
     * @brief Convert payload to JSON string
     */
    std::string toJson() const;

    /**
     * @brief Parse JSON string to payload
     */
    static JwtPayload fromJson(const std::string& json);

    /**
     * @brief Decode JWT token and extract payload
     */
    static JwtPayload fromToken(const std::string& token);
};

/**
 * @brief Session information
 */
struct Session {
    int id;
    int userId;
    std::string deviceName;
    std::string deviceType;
    std::string ipAddress;
    int64_t expiresAt;
    int64_t lastUsedAt;
    int64_t createdAt;

    Session() : id(0), userId(0), expiresAt(0), lastUsedAt(0), createdAt(0) {}
};

// ============================================================================
// Authentication Manager Class
// ============================================================================

/**
 * @brief Authentication Manager
 *
 * Handles user registration, login, token management, and session handling
 * Supports both MySQL (multi-user) and SQLite (single-user) modes
 *
 * Usage:
 * @code
 * auto& authManager = AuthManager::getInstance();
 * authManager.initialize(&dbManager);
 * authManager.setJwtSecret("your-secret-key");
 *
 * // Register user
 * RegistrationData data;
 * data.username = "john_doe";
 * data.email = "john@example.com";
 * data.password = "SecurePass123!";
 * AuthResult result = authManager.registerUser(data);
 * @endcode
 */
class AuthManager {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to AuthManager instance
     */
    static AuthManager& getInstance();

    /**
     * @brief Initialize with database manager
     * @param dbManager Pointer to database manager (MySQL or SQLite)
     */
    void initialize(DatabaseManager* dbManager);

    /**
     * @brief Initialize with SQLite manager (convenience method)
     * @param sqliteManager Pointer to SQLite manager
     */
    void initialize(SqliteManager* sqliteManager);

    // ========================================================================
    // Authentication Operations
    // ========================================================================

    /**
     * @brief Register new user
     * @param data Registration information
     * @return AuthResult with tokens on success
     *
     * Error codes:
     * - INVALID_INPUT: Missing required fields
     * - WEAK_PASSWORD: Password doesn't meet strength requirements
     * - EMAIL_EXISTS: Email already registered
     * - USERNAME_EXISTS: Username already taken
     * - RATE_LIMITED: Too many registration attempts
     */
    AuthResult registerUser(const RegistrationData& data);

    /**
     * @brief Login user
     * @param credentials Login credentials
     * @return AuthResult with tokens on success
     *
     * Error codes:
     * - INVALID_CREDENTIALS: Wrong email or password
     * - ACCOUNT_LOCKED: Account temporarily locked
     * - ACCOUNT_INACTIVE: Account has been deactivated
     * - RATE_LIMITED: Too many login attempts
     */
    AuthResult login(const LoginCredentials& credentials);

    /**
     * @brief Logout user and invalidate session
     * @param refreshToken Refresh token to invalidate
     * @return true on success
     */
    bool logout(const std::string& refreshToken);

    /**
     * @brief Refresh access token
     * @param refreshToken Valid refresh token
     * @return New access token or empty string on failure
     */
    std::string refreshAccessToken(const std::string& refreshToken);

    /**
     * @brief Verify access token and return user info
     * @param accessToken JWT access token
     * @return User if valid, std::nullopt otherwise
     */
    std::optional<User> verifyToken(const std::string& accessToken);

    /**
     * @brief Change user password
     * @param userId User ID
     * @param oldPassword Current password
     * @param newPassword New password
     * @return true on success
     */
    bool changePassword(int userId, const std::string& oldPassword,
                       const std::string& newPassword);

    /**
     * @brief Request password reset
     * @param email User email
     * @return Reset token or empty string
     */
    std::string requestPasswordReset(const std::string& email);

    /**
     * @brief Reset password with token
     * @param resetToken Password reset token
     * @param newPassword New password
     * @return true on success
     */
    bool resetPassword(const std::string& resetToken, const std::string& newPassword);

    // ========================================================================
    // User Management
    // ========================================================================

    /**
     * @brief Get user by ID
     * @param userId User ID
     * @return User if found, std::nullopt otherwise
     */
    std::optional<User> getUserById(int userId);

    /**
     * @brief Get user by email
     * @param email User email
     * @return User if found, std::nullopt otherwise
     */
    std::optional<User> getUserByEmail(const std::string& email);

    /**
     * @brief Get user by username
     * @param username Username
     * @return User if found, std::nullopt otherwise
     */
    std::optional<User> getUserByUsername(const std::string& username);

    /**
     * @brief Update user profile
     * @param userId User ID
     * @param fullName Full name
     * @param affiliation Affiliation
     * @return true on success
     */
    bool updateUserProfile(int userId, const std::string& fullName,
                          const std::string& affiliation);

    /**
     * @brief Deactivate user account
     * @param userId User ID
     * @return true on success
     */
    bool deactivateUser(int userId);

    /**
     * @brief Reactivate user account
     * @param userId User ID
     * @return true on success
     */
    bool reactivateUser(int userId);

    // ========================================================================
    // Session Management
    // ========================================================================

    /**
     * @brief Create user session
     * @param userId User ID
     * @param deviceName Device identifier
     * @param ipAddress IP address
     * @param userAgent User agent string
     * @return Session tokens
     */
    AuthTokens createSession(int userId, const std::string& deviceName,
                            const std::string& ipAddress,
                            const std::string& userAgent);

    /**
     * @brief Validate session
     * @param refreshToken Refresh token
     * @return User ID if valid, 0 otherwise
     */
    int validateSession(const std::string& refreshToken);

    /**
     * @brief Get active sessions for user
     * @param userId User ID
     * @return List of sessions
     */
    std::vector<Session> getActiveSessions(int userId);

    /**
     * @brief Invalidate all user sessions
     * @param userId User ID
     * @return true on success
     */
    bool invalidateAllSessions(int userId);

    /**
     * @brief Invalidate specific session
     * @param sessionId Session ID
     * @return true on success
     */
    bool invalidateSession(int sessionId);

    // ========================================================================
    // Security
    // ========================================================================

    /**
     * @brief Check rate limiting for login attempts
     * @param identifier Email or IP address
     * @return true if rate limited
     */
    bool isRateLimited(const std::string& identifier);

    /**
     * @brief Record login attempt
     * @param identifier Email or IP address
     * @param success Whether login was successful
     * @param ipAddress IP address
     * @param userAgent User agent string
     */
    void recordLoginAttempt(const std::string& identifier, bool success,
                           const std::string& ipAddress,
                           const std::string& userAgent);

    /**
     * @brief Lock user account after failed attempts
     * @param userId User ID
     * @param lockDurationMinutes Duration of lock in minutes
     * @return true on success
     */
    bool lockUserAccount(int userId, int lockDurationMinutes = 30);

    /**
     * @brief Unlock user account
     * @param userId User ID
     * @return true on success
     */
    bool unlockUserAccount(int userId);

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set JWT secret key
     * @param secret Secret key for signing tokens
     *
     * IMPORTANT: This must be set before any auth operations
     * Use a strong, random secret in production
     */
    void setJwtSecret(const std::string& secret);

    /**
     * @brief Set token expiration times
     * @param accessMinutes Access token expiry in minutes (default: 15)
     * @param refreshDays Refresh token expiry in days (default: 30)
     */
    void setTokenExpiry(int accessMinutes, int refreshDays);

    /**
     * @brief Set rate limiting parameters
     * @param maxAttempts Maximum attempts allowed
     * @param windowMs Time window in milliseconds
     * @param lockDurationMinutes Account lock duration
     */
    void setRateLimitingParams(int maxAttempts, int windowMs, int lockDurationMinutes);

    /**
     * @brief Enable/disable authentication
     * @param enabled true to enable, false to disable
     *
     * When disabled, all requests are allowed (useful for maintenance)
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief Check if authentication is enabled
     * @return true if enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Get JWT secret
     * @return JWT secret key
     */
    const std::string& getJwtSecret() const { return jwtSecret_; }

private:
    // ========================================================================
    // Constructors
    // ========================================================================

    AuthManager() = default;
    ~AuthManager() = default;

    // Prevent copying
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Validate password strength
     * @param password Password to validate
     * @return true if password meets requirements
     *
     * Requirements:
     * - At least 8 characters
     * - Contains uppercase letter
     * - Contains lowercase letter
     * - Contains number
     * - Contains special character
     */
    bool validatePasswordStrength(const std::string& password);

    /**
     * @brief Validate email format
     * @param email Email to validate
     * @return true if email is valid
     */
    bool isValidEmail(const std::string& email);

    /**
     * @brief Validate username format
     * @param username Username to validate
     * @return true if username is valid
     *
     * Requirements:
     * - 3-30 characters
     * - Alphanumeric, underscores, hyphens only
     */
    bool isValidUsername(const std::string& username);

    /**
     * @brief Hash password with salt
     * @param password Plain text password
     * @param salt Salt for hashing
     * @return Password hash
     */
    std::string hashPassword(const std::string& password, const std::string& salt);

    /**
     * @brief Generate random salt
     * @param saltLength Length of salt in bytes
     * @return Hex-encoded salt string
     */
    std::string generateSalt(int saltLength = 32);

    /**
     * @brief Generate JWT token
     * @param payload Token payload
     * @return JWT token string
     */
    std::string generateJwtToken(const JwtPayload& payload);

    /**
     * @brief Verify JWT token
     * @param token JWT token string
     * @return JwtPayload if valid, std::nullopt otherwise
     */
    std::optional<JwtPayload> verifyJwtToken(const std::string& token);

    /**
     * @brief Generate secure random token
     * @param length Token length in bytes
     * @return Hex-encoded token string
     */
    std::string generateSecureToken(int length = 32);

    /**
     * @brief Hash token for storage
     * @param token Token to hash
     * @return Hashed token
     */
    std::string hashToken(const std::string& token);

    // ========================================================================
    // Database Operations
    // ========================================================================

    /**
     * @brief Create user in database
     * @param data Registration data
     * @param passwordHash Password hash
     * @param salt Salt used for hashing
     * @return User ID if successful, 0 otherwise
     */
    int createUserInDb(const RegistrationData& data, const std::string& passwordHash,
                       const std::string& salt);

    /**
     * @brief Get user from database by email
     * @param email User email
     * @return User if found, std::nullopt otherwise
     */
    std::optional<User> getUserFromDbByEmail(const std::string& email);

    /**
     * @brief Get user from database by ID
     * @param userId User ID
     * @return User if found, std::nullopt otherwise
     */
    std::optional<User> getUserFromDbById(int userId);

    /**
     * @brief Update user's last login
     * @param userId User ID
     * @param ipAddress IP address
     * @return true on success
     */
    bool updateUserLastLogin(int userId, const std::string& ipAddress);

    /**
     * @brief Increment login attempt counter
     * @param email User email
     * @return true on success
     */
    bool incrementLoginAttempts(const std::string& email);

    /**
     * @brief Reset login attempt counter
     * @param email User email
     * @return true on success
     */
    bool resetLoginAttempts(const std::string& email);

    /**
     * @brief Check if user account is locked
     * @param email User email
     * @return true if locked
     */
    bool isAccountLocked(const std::string& email);

    /**
     * @brief Save session to database
     * @param userId User ID
     * @param refreshToken Refresh token
     * @param accessTokenHash Access token hash
     * @param deviceName Device name
     * @param deviceType Device type
     * @param ipAddress IP address
     * @param userAgent User agent
     * @return Session ID if successful, 0 otherwise
     */
    int saveSessionToDb(int userId, const std::string& refreshToken,
                        const std::string& accessTokenHash,
                        const std::string& deviceName,
                        const std::string& deviceType,
                        const std::string& ipAddress,
                        const std::string& userAgent);

    /**
     * @brief Remove session from database
     * @param refreshToken Refresh token
     * @return true on success
     */
    bool removeSessionFromDb(const std::string& refreshToken);

    /**
     * @brief Get session by refresh token
     * @param refreshToken Refresh token
     * @return Session if found, std::nullopt otherwise
     */
    std::optional<Session> getSessionFromDb(const std::string& refreshToken);

    /**
     * @brief Update session's last used timestamp
     * @param sessionId Session ID
     * @return true on success
     */
    bool updateSessionLastUsed(int sessionId);

    /**
     * @brief Clean up expired sessions
     * @return Number of sessions cleaned
     */
    int cleanupExpiredSessions();

    /**
     * @brief Clean up old login attempts
     * @return Number of attempts cleaned
     */
    int cleanupOldLoginAttempts();

    // ========================================================================
    // Members
    // ========================================================================

    // Database managers
    DatabaseManager* dbManager_{nullptr};
    SqliteManager* sqliteManager_{nullptr};
    bool useMySql_{true};

    // Configuration
    std::string jwtSecret_;
    int accessTokenExpiry_{15};     // minutes
    int refreshTokenExpiry_{30};    // days
    int maxLoginAttempts_{5};
    int rateLimitWindowMs_{60000};   // 1 minute
    int lockDurationMinutes_{30};
    bool enabled_{true};

    // Security helpers
    std::unique_ptr<RateLimiter> rateLimiter_;

    // Cache
    std::map<int, User> userCache_;
    int64_t cacheExpiry_{0};
    static const int64_t CACHE_TTL_MS = 300000; // 5 minutes
};

// ============================================================================
// Inline Functions
// ============================================================================

inline AuthManager& AuthManager::getInstance() {
    static AuthManager instance;
    return instance;
}

} // namespace PaperCrawler
