/**
 * Authentication System Unit Tests
 *
 * Tests for AuthManager, JwtUtils, PasswordHasher, and RateLimiter
 * Uses Google Test framework
 *
 * Compile with: g++ -std=c++17 test_auth.cpp -o test_auth -lgtest -pthread
 * Run with: ./test_auth
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "auth/AuthManager.hpp"
#include "auth/JwtUtils.hpp"
#include "auth/PasswordHasher.hpp"
#include "auth/RateLimiter.hpp"
#include "database/SqliteManager.hpp"
#include <chrono>
#include <thread>

using namespace PaperCrawler;

// ============================================================================
// Test Fixture
// ============================================================================

class AuthManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup in-memory test database
        SqliteConfig config;
        config.databasePath = ":memory:"; // In-memory database for testing

        sqliteManager_.initialize(config);

        // Run schema creation
        sqliteManager_.execute(R"(
            CREATE TABLE users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                email TEXT UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                salt TEXT NOT NULL,
                full_name TEXT,
                role TEXT DEFAULT 'user',
                is_active INTEGER DEFAULT 1,
                login_attempts INTEGER DEFAULT 0,
                created_at INTEGER DEFAULT (strftime('%s', 'now'))
            );

            CREATE TABLE user_sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                refresh_token TEXT NOT NULL,
                access_token_hash TEXT NOT NULL,
                expires_at INTEGER NOT NULL,
                created_at INTEGER DEFAULT (strftime('%s', 'now'))
            );

            CREATE TABLE login_attempts (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                identifier TEXT NOT NULL,
                success INTEGER DEFAULT 0,
                ip_address TEXT,
                created_at INTEGER DEFAULT (strftime('%s', 'now'))
            );
        )");

        // Initialize auth manager
        authManager_.initialize(&sqliteManager_);
        authManager_.setJwtSecret("test-secret-key-for-unit-testing");
        authManager_.setEnabled(true);
    }

    void TearDown() override {
        sqliteManager_.shutdown();
    }

    // Helper: Create test user
    int createTestUser(const std::string& email, const std::string& password) {
        RegistrationData data;
        data.username = "testuser_" + email;
        data.email = email;
        data.password = password;
        data.fullName = "Test User";

        AuthResult result = authManager_.registerUser(data);
        if (result.success) {
            return result.user.id;
        }
        return 0;
    }

    SqliteManager sqliteManager_;
    AuthManager authManager_;
};

// ============================================================================
// Registration Tests
// ============================================================================

TEST_F(AuthManagerTest, RegisterUser_Success) {
    RegistrationData data;
    data.username = "john_doe";
    data.email = "john@example.com";
    data.password = "SecurePass123!";
    data.fullName = "John Doe";

    AuthResult result = authManager_.registerUser(data);

    EXPECT_TRUE(result.success);
    EXPECT_GT(result.user.id, 0);
    EXPECT_EQ(result.user.username, "john_doe");
    EXPECT_EQ(result.user.email, "john@example.com");
    EXPECT_EQ(result.user.role, "user");
    EXPECT_TRUE(result.user.isActive);
    EXPECT_FALSE(result.tokens.accessToken.empty());
    EXPECT_FALSE(result.tokens.refreshToken.empty());
}

TEST_F(AuthManagerTest, RegisterUser_DuplicateEmail_Fails) {
    // Create first user
    createTestUser("duplicate@example.com", "Password123!");

    // Try to create second user with same email
    RegistrationData data;
    data.username = "different_user";
    data.email = "duplicate@example.com"; // Same email
    data.password = "Password123!";

    AuthResult result = authManager_.registerUser(data);

    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.errorCode.empty());
}

TEST_F(AuthManagerTest, RegisterUser_WeakPassword_Fails) {
    RegistrationData data;
    data.username = "weakpass";
    data.email = "weak@example.com";
    data.password = "weak"; // Too short

    AuthResult result = authManager_.registerUser(data);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.errorCode.find("WEAK_PASSWORD") != std::string::npos ||
                result.errorCode.find("INVALID_INPUT") != std::string::npos);
}

// ============================================================================
// Login Tests
// ============================================================================

TEST_F(AuthManagerTest, Login_Success) {
    // Register user first
    std::string email = "login@example.com";
    std::string password = "LoginPass123!";
    createTestUser(email, password);

    // Login
    LoginCredentials creds;
    creds.email = email;
    creds.password = password;
    creds.ipAddress = "127.0.0.1";

    AuthResult result = authManager_.login(creds);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.user.email, email);
    EXPECT_FALSE(result.tokens.accessToken.empty());
    EXPECT_FALSE(result.tokens.refreshToken.empty());
}

TEST_F(AuthManagerTest, Login_WrongPassword_Fails) {
    // Register user first
    createTestUser("wrongpass@example.com", "CorrectPass123!");

    // Login with wrong password
    LoginCredentials creds;
    creds.email = "wrongpass@example.com";
    creds.password = "WrongPass123!";
    creds.ipAddress = "127.0.0.1";

    AuthResult result = authManager_.login(creds);

    EXPECT_FALSE(result.success);
}

TEST_F(AuthManagerTest, Login_NonExistentUser_Fails) {
    LoginCredentials creds;
    creds.email = "nonexistent@example.com";
    creds.password = "AnyPass123!";
    creds.ipAddress = "127.0.0.1";

    AuthResult result = authManager_.login(creds);

    EXPECT_FALSE(result.success);
}

// ============================================================================
// Token Verification Tests
// ============================================================================

TEST_F(AuthManagerTest, VerifyToken_ValidToken_Success) {
    // Register and login
    std::string email = "token@example.com";
    createTestUser(email, "TokenPass123!");

    LoginCredentials creds;
    creds.email = email;
    creds.password = "TokenPass123!";
    creds.ipAddress = "127.0.0.1";

    AuthResult loginResult = authManager_.login(creds);
    ASSERT_TRUE(loginResult.success);

    // Verify token
    std::optional<User> user = authManager_.verifyToken(loginResult.tokens.accessToken);

    EXPECT_TRUE(user.has_value());
    EXPECT_EQ(user->id, loginResult.user.id);
    EXPECT_EQ(user->email, email);
}

TEST_F(AuthManagerTest, VerifyToken_InvalidToken_Fails) {
    std::optional<User> user = authManager_.verifyToken("invalid.token.here");

    EXPECT_FALSE(user.has_value());
}

// ============================================================================
// Token Refresh Tests
// ============================================================================

TEST_F(AuthManagerTest, RefreshToken_ValidRefreshToken_Success) {
    // Register and login
    std::string email = "refresh@example.com";
    createTestUser(email, "RefreshPass123!");

    LoginCredentials creds;
    creds.email = email;
    creds.password = "RefreshPass123!";
    creds.ipAddress = "127.0.0.1";

    AuthResult loginResult = authManager_.login(creds);
    ASSERT_TRUE(loginResult.success);

    // Refresh token
    std::string newAccessToken = authManager_.refreshAccessToken(loginResult.tokens.refreshToken);

    EXPECT_FALSE(newAccessToken.empty());
    EXPECT_NE(newAccessToken, loginResult.tokens.accessToken);

    // Verify new token works
    std::optional<User> user = authManager_.verifyToken(newAccessToken);
    EXPECT_TRUE(user.has_value());
}

TEST_F(AuthManagerTest, RefreshToken_InvalidToken_Fails) {
    std::string newAccessToken = authManager_.refreshAccessToken("invalid_refresh_token");

    EXPECT_TRUE(newAccessToken.empty());
}

// ============================================================================
// Rate Limiting Tests
// ============================================================================

TEST_F(AuthManagerTest, RateLimit_ExceedAttempts_Locks) {
    RateLimiter limiter(5, 60000, 300000); // 5 attempts, 1 min window
    std::string identifier = "test@example.com";

    // Make 5 failed attempts
    for (int i = 0; i < 5; i++) {
        limiter.recordFailedAttempt(identifier);
    }

    // Should be rate limited now
    EXPECT_TRUE(limiter.isRateLimited(identifier));

    // Get remaining attempts
    int remaining = limiter.getRemainingAttempts(identifier);
    EXPECT_EQ(remaining, 0);
}

TEST_F(AuthManagerTest, RateLimit_SuccessfulAttempt_Resets) {
    RateLimiter limiter(5, 60000, 300000);
    std::string identifier = "success@example.com";

    // Make 4 failed attempts
    for (int i = 0; i < 4; i++) {
        limiter.recordFailedAttempt(identifier);
    }

    // Make successful attempt
    limiter.recordAttempt(identifier, true);

    // Should have remaining attempts now
    int remaining = limiter.getRemainingAttempts(identifier);
    EXPECT_GT(remaining, 0);
}

TEST_F(AuthManagerTest, RateLimit_TimeWindow_Resets) {
    RateLimiter limiter(2, 100, 1000); // 2 attempts, 100ms window
    std::string identifier = "time@example.com";

    // Make 2 attempts
    limiter.recordFailedAttempt(identifier);
    limiter.recordFailedAttempt(identifier);

    EXPECT_TRUE(limiter.isRateLimited(identifier));

    // Wait for window to expire
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Should no longer be rate limited
    EXPECT_FALSE(limiter.isRateLimited(identifier));
}

// ============================================================================
// Password Hashing Tests
// ============================================================================

TEST(PasswordHasherTest, HashPassword_ConsistentResults) {
    std::string password = "TestPassword123!";
    std::string salt;

    std::string hash1 = PasswordHasher::hashPassword(password, salt);
    std::string hash2 = PasswordHasher::hashPassword(password, salt);

    EXPECT_EQ(hash1, hash2);
}

TEST(PasswordHasherTest, VerifyPassword_CorrectPassword_Success) {
    std::string password = "CorrectPassword123!";
    std::string salt;

    std::string hash = PasswordHasher::hashPassword(password, salt);

    EXPECT_TRUE(PasswordHasher::verifyPassword(password, hash, salt));
}

TEST(PasswordHasherTest, VerifyPassword_WrongPassword_Fails) {
    std::string password = "CorrectPassword123!";
    std::string wrongPassword = "WrongPassword123!";
    std::string salt;

    std::string hash = PasswordHasher::hashPassword(password, salt);

    EXPECT_FALSE(PasswordHasher::verifyPassword(wrongPassword, hash, salt));
}

TEST(PasswordHasherTest, GenerateSalt_UniqueEachTime) {
    std::string salt1 = PasswordHasher::generateSalt();
    std::string salt2 = PasswordHasher::generateSalt();

    EXPECT_NE(salt1, salt2);
    EXPECT_EQ(salt1.length(), 64); // 32 bytes = 64 hex chars
    EXPECT_EQ(salt2.length(), 64);
}

TEST(PasswordHasherTest, ValidateStrength_StrongPassword_ReturnsTrue) {
    EXPECT_TRUE(PasswordHasher::validateStrength("StrongPass123!"));
    EXPECT_TRUE(PasswordHasher::validateStrength("MySecur3P@ssw0rd"));
}

TEST(PasswordHasherTest, ValidateStrength_WeakPassword_ReturnsFalse) {
    EXPECT_FALSE(PasswordHasher::validateStrength("weak")); // Too short
    EXPECT_FALSE(PasswordHasher::validateStrength("nouppercase123!")); // No uppercase
    EXPECT_FALSE(PasswordHasher::validateStrength("NOLOWERCASE123!")); // No lowercase
    EXPECT_FALSE(PasswordHasher::validateStrength("NoNumbers!")); // No digits
    EXPECT_FALSE(PasswordHasher::validateStrength("NoSpecial123")); // No special chars
}

// ============================================================================
// JWT Tests
// ============================================================================

TEST(JwtUtilsTest, GenerateAndVerifyToken_Success) {
    JwtPayload payload;
    payload.userId = 123;
    payload.username = "testuser";
    payload.email = "test@example.com";
    payload.role = "user";
    payload.issuedAt = std::chrono::system_clock::now().time_since_epoch() /
                       std::chrono::seconds(1);
    payload.expiresAt = payload.issuedAt + 900; // 15 minutes

    std::string secret = "test-secret";
    std::string token = JwtUtils::generateToken(payload, secret);

    EXPECT_FALSE(token.empty());
    EXPECT_NE(token.find('.'), std::string::npos); // JWT has 3 parts

    std::optional<JwtPayload> decoded = JwtUtils::verifyToken(token, secret);

    EXPECT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->userId, 123);
    EXPECT_EQ(decoded->username, "testuser");
    EXPECT_EQ(decoded->email, "test@example.com");
}

TEST(JwtUtilsTest, VerifyToken_WrongSecret_Fails) {
    JwtPayload payload;
    payload.userId = 123;
    payload.username = "testuser";
    payload.issuedAt = std::chrono::system_clock::now().time_since_epoch() /
                       std::chrono::seconds(1);
    payload.expiresAt = payload.issuedAt + 900;

    std::string secret1 = "secret1";
    std::string secret2 = "secret2";

    std::string token = JwtUtils::generateToken(payload, secret1);
    std::optional<JwtPayload> decoded = JwtUtils::verifyToken(token, secret2);

    EXPECT_FALSE(decoded.has_value());
}

TEST(JwtUtilsTest, VerifyToken_ExpiredToken_Fails) {
    JwtPayload payload;
    payload.userId = 123;
    payload.username = "testuser";
    payload.issuedAt = std::chrono::system_clock::now().time_since_epoch() /
                       std::chrono::seconds(1);
    payload.expiresAt = payload.issuedAt - 100; // Already expired

    std::string secret = "test-secret";
    std::string token = JwtUtils::generateToken(payload, secret);
    std::optional<JwtPayload> decoded = JwtUtils::verifyToken(token, secret);

    EXPECT_FALSE(decoded.has_value());
}

// ============================================================================
// Logout Tests
// ============================================================================

TEST_F(AuthManagerTest, Logout_InvalidatesSession) {
    // Register and login
    std::string email = "logout@example.com";
    createTestUser(email, "LogoutPass123!");

    LoginCredentials creds;
    creds.email = email;
    creds.password = "LogoutPass123!";
    creds.ipAddress = "127.0.0.1";

    AuthResult loginResult = authManager_.login(creds);
    ASSERT_TRUE(loginResult.success);

    // Logout
    bool logoutSuccess = authManager_.logout(loginResult.tokens.refreshToken);
    EXPECT_TRUE(logoutSuccess);

    // Token should no longer be valid
    std::optional<User> user = authManager_.verifyToken(loginResult.tokens.accessToken);
    EXPECT_FALSE(user.has_value());
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
