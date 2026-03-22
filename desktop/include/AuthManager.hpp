#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <memory>

// Forward declarations
class ApiManager;

/**
 * @brief User information for desktop client
 */
struct DesktopUser {
    int id;
    QString username;
    QString email;
    QString fullName;
    QString role;
    bool isActive;
    QString avatarUrl;

    DesktopUser()
        : id(0)
        , isActive(false)
    {}
};

/**
 * @brief Authentication tokens for desktop client
 */
struct DesktopAuthTokens {
    QString accessToken;
    QString refreshToken;
    qint64 expiresAt;

    DesktopAuthTokens()
        : expiresAt(0)
    {}
};

/**
 * @brief Authentication result for desktop client
 */
struct DesktopAuthResult {
    bool success;
    QString error;
    DesktopUser user;
    DesktopAuthTokens tokens;

    DesktopAuthResult()
        : success(false)
    {}
};

/**
 * @brief Desktop Authentication Manager
 *
 * Handles user authentication for desktop Qt client
 * Integrates with backend REST API
 * Uses QSettings for secure token storage
 *
 * Features:
 * - User login/logout
 * - Token management with auto-refresh
 * - Secure credential storage
 * - Integration with ApiManager
 * - Signal-based notifications
 *
 * Usage:
 * @code
 * auto* authManager = new AuthManager(this);
 * authManager->setApiManager(apiManager);
 *
 * // Connect signals
 * connect(authManager, &AuthManager::loginSuccess,
 *         this, &MainWindow::onLoginSuccess);
 *
 * // Login
 * authManager->login("user@example.com", "password");
 * @endcode
 */
class AuthManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit AuthManager(QObject* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~AuthManager();

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Set base URL for API requests
     * @param url Base URL (e.g., "http://localhost:8080")
     */
    void setBaseUrl(const QString& url);

    /**
     * @brief Set API manager
     * @param apiManager Pointer to ApiManager instance
     */
    void setApiManager(ApiManager* apiManager);

    /**
     * @brief Get API manager
     * @return Pointer to ApiManager instance
     */
    ApiManager* apiManager() const { return apiManager_; }

    // ========================================================================
    // Authentication Operations
    // ========================================================================

    /**
     * @brief Login user
     * @param email User email
     * @param password User password
     *
     * Emits loginSuccess() or loginFailed() signal when complete
     */
    void login(const QString& email, const QString& password);

    /**
     * @brief Register new user
     * @param username Username
     * @param email Email address
     * @param password Password
     * @param fullName Full name (optional)
     *
     * Emits registerSuccess() or registerFailed() signal when complete
     */
    void registerUser(const QString& username, const QString& email,
                     const QString& password, const QString& fullName = "");

    /**
     * @brief Logout current user
     *
     * Emits logoutSuccess() or logoutFailed() signal when complete
     */
    void logout();

    /**
     * @brief Refresh access token
     *
     * Called automatically by timer, but can be called manually
     * Emits tokenRefreshed() or tokenRefreshFailed() signal when complete
     */
    void refreshToken();

    // ========================================================================
    // User Operations
    // ========================================================================

    /**
     * @brief Fetch current user info
     *
     * Emits userFetched() or userFetchFailed() signal when complete
     */
    void fetchCurrentUser();

    /**
     * @brief Update user profile
     * @param fullName Full name
     * @param affiliation Affiliation
     *
     * Emits profileUpdated() or profileUpdateFailed() signal when complete
     */
    void updateProfile(const QString& fullName, const QString& affiliation);

    /**
     * @brief Change password
     * @param oldPassword Current password
     * @param newPassword New password
     *
     * Emits passwordChanged() or passwordChangeFailed() signal when complete
     */
    void changePassword(const QString& oldPassword, const QString& newPassword);

    // ========================================================================
    // State
    // ========================================================================

    /**
     * @brief Check if user is authenticated
     * @return true if authenticated
     */
    bool isAuthenticated() const { return authenticated_; }

    /**
     * @brief Get current user
     * @return Current user info
     */
    DesktopUser getCurrentUser() const { return user_; }

    /**
     * @brief Get current tokens
     * @return Current tokens
     */
    DesktopAuthTokens getTokens() const { return tokens_; }

    /**
     * @brief Get access token
     * @return Access token string
     */
    QString getAccessToken() const { return tokens_.accessToken; }

    /**
     * @brief Set tokens (for persistence)
     * @param tokens Tokens to set
     */
    void setTokens(const DesktopAuthTokens& tokens);

    /**
     * @brief Load saved authentication state
     * @return true if successfully loaded
     */
    bool loadSavedState();

    /**
     * @brief Clear saved authentication state
     */
    void clearSavedState();

signals:
    // ========================================================================
    // Authentication Signals
    // ========================================================================

    /**
     * @brief Emitted when login succeeds
     * @param user Logged in user
     */
    void loginSuccess(const DesktopUser& user);

    /**
     * @brief Emitted when login fails
     * @param error Error message
     */
    void loginFailed(const QString& error);

    /**
     * @brief Emitted when registration succeeds
     * @param user Registered user
     */
    void registerSuccess(const DesktopUser& user);

    /**
     * @brief Emitted when registration fails
     * @param error Error message
     */
    void registerFailed(const QString& error);

    /**
     * @brief Emitted when logout succeeds
     */
    void logoutSuccess();

    /**
     * @brief Emitted when logout fails
     * @param error Error message
     */
    void logoutFailed(const QString& error);

    // ========================================================================
    // Token Signals
    // ========================================================================

    /**
     * @brief Emitted when token is refreshed
     * @param newAccessToken New access token
     */
    void tokenRefreshed(const QString& newAccessToken);

    /**
     * @brief Emitted when token refresh fails
     * @param error Error message
     */
    void tokenRefreshFailed(const QString& error);

    // ========================================================================
    // User Signals
    // ========================================================================

    /**
     * @brief Emitted when user info is fetched
     * @param user User info
     */
    void userFetched(const DesktopUser& user);

    /**
     * @brief Emitted when user fetch fails
     * @param error Error message
     */
    void userFetchFailed(const QString& error);

    /**
     * @brief Emitted when profile is updated
     * @param user Updated user info
     */
    void profileUpdated(const DesktopUser& user);

    /**
     * @brief Emitted when profile update fails
     * @param error Error message
     */
    void profileUpdateFailed(const QString& error);

    /**
     * @brief Emitted when password is changed
     */
    void passwordChanged();

    /**
     * @brief Emitted when password change fails
     * @param error Error message
     */
    void passwordChangeFailed(const QString& error);

    // ========================================================================
    // Authentication State Change
    // ========================================================================

    /**
     * @brief Emitted when authentication state changes
     * @param authenticated true if now authenticated
     */
    void authenticationChanged(bool authenticated);

private slots:
    // ========================================================================
    // Network Reply Slots
    // ========================================================================

    void onLoginReply();
    void onRegisterReply();
    void onLogoutReply();
    void onRefreshReply();
    void onUserFetchReply();
    void onProfileUpdateReply();
    void onPasswordChangeReply();

private:
    // ========================================================================
    // Helper Methods
    // ========================================================================

    /**
     * @brief Save tokens to QSettings
     */
    void saveTokens();

    /**
     * @brief Load tokens from QSettings
     * @return true if tokens were loaded
     */
    bool loadTokens();

    /**
     * @brief Clear tokens from QSettings
     */
    void clearTokens();

    /**
     * @brief Schedule automatic token refresh
     */
    void scheduleTokenRefresh();

    /**
     * @brief Create network request
     * @param endpoint API endpoint
     * @return QNetworkRequest with auth header
     */
    QNetworkRequest createRequest(const QString& endpoint);

    /**
     * @brief Parse user from JSON
     * @param jsonObject JSON object
     * @return Parsed user
     */
    DesktopUser parseUser(const QJsonObject& jsonObject);

    /**
     * @brief Parse tokens from JSON
     * @param jsonObject JSON object
     * @return Parsed tokens
     */
    DesktopAuthTokens parseTokens(const QJsonObject& jsonObject);

    /**
     * @brief Handle authentication error
     * @param reply Network reply
     */
    void handleError(QNetworkReply* reply);

    // ========================================================================
    // Members
    // ========================================================================

    ApiManager* apiManager_{nullptr};
    QString baseUrl_;

    // Authentication state
    bool authenticated_{false};
    DesktopUser user_;
    DesktopAuthTokens tokens_;

    // Settings for token storage
    QSettings* settings_{nullptr};

    // Network replies
    QNetworkReply* loginReply_{nullptr};
    QNetworkReply* registerReply_{nullptr};
    QNetworkReply* logoutReply_{nullptr};
    QNetworkReply* refreshReply_{nullptr};
    QNetworkReply* userFetchReply_{nullptr};
    QNetworkReply* profileUpdateReply_{nullptr};
    QNetworkReply* passwordChangeReply_{nullptr};

    // Token refresh timer
    QTimer* refreshTimer_{nullptr};
};
