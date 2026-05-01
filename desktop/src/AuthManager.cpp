/**
 * Desktop Authentication Manager Implementation
 *
 * Qt/C++ implementation for desktop client authentication
 * Integrates with backend REST API
 */

#include "AuthManager.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QTimer>
#include <QCryptographicHash>

AuthManager::AuthManager(QObject* parent)
    : QObject(parent)
    , networkManager_(new QNetworkAccessManager(this))
    , authenticated_(false)
    , settings_(new QSettings("PaperCrawler", "DesktopClient", this))
    , refreshTimer_(new QTimer(this))
{
    connect(refreshTimer_, &QTimer::timeout, this, &AuthManager::refreshToken);
    loadSavedState();
}

AuthManager::~AuthManager() {
    // Cleanup
    if (refreshTimer_->isActive()) {
        refreshTimer_->stop();
    }
}

// ============================================================================
// Configuration
// ============================================================================

void AuthManager::setBaseUrl(const QString& url) {
    baseUrl_ = url;
}

// ============================================================================
// Authentication Operations
// ============================================================================

void AuthManager::login(const QString& email, const QString& password) {

    // Create request
    QNetworkRequest request;
    request.setUrl(baseUrl_ + "/api/auth/login");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Create JSON body
    QJsonObject body;
    body["email"] = email;
    body["password"] = password;

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    // Send request
    loginReply_ = networkManager_->post(request, data);

    connect(loginReply_, &QNetworkReply::finished, this, &AuthManager::onLoginReply);
}

void AuthManager::registerUser(const QString& username, const QString& email,
                              const QString& password, const QString& fullName) {

    // Create request
    QNetworkRequest request;
    request.setUrl(baseUrl_ + "/api/auth/register");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Create JSON body
    QJsonObject body;
    body["username"] = username;
    body["email"] = email;
    body["password"] = password;
    if (!fullName.isEmpty()) {
        body["fullName"] = fullName;
    }

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    // Send request
    registerReply_ = networkManager_->post(request, data);

    connect(registerReply_, &QNetworkReply::finished, this, &AuthManager::onRegisterReply);
}

void AuthManager::logout() {

    // Create request
    QNetworkRequest request;
    request.setUrl(baseUrl_ + "/api/auth/logout");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Add auth header
    if (!tokens_.accessToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + tokens_.accessToken.toUtf8());
    }

    // Create JSON body
    QJsonObject body;
    body["refreshToken"] = tokens_.refreshToken;

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    // Send request
    logoutReply_ = networkManager_->post(request, data);

    connect(logoutReply_, &QNetworkReply::finished, this, &AuthManager::onLogoutReply);
}

void AuthManager::refreshToken() {
    if (tokens_.refreshToken.isEmpty()) {
        emit tokenRefreshFailed("No refresh token available");
        return;
    }

    // Create request
    QNetworkRequest request;
    request.setUrl(baseUrl_ + "/api/auth/refresh");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Create JSON body
    QJsonObject body;
    body["refreshToken"] = tokens_.refreshToken;

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    // Send request
    refreshReply_ = networkManager_->post(request, data);

    connect(refreshReply_, &QNetworkReply::finished, this, &AuthManager::onRefreshReply);
}

// ============================================================================
// User Operations
// ============================================================================

void AuthManager::fetchCurrentUser() {

    QNetworkRequest request = createRequest("/api/auth/me");
    userFetchReply_ = networkManager_->get(request);

    connect(userFetchReply_, &QNetworkReply::finished, this, &AuthManager::onUserFetchReply);
}

void AuthManager::updateProfile(const QString& fullName, const QString& affiliation) {

    QNetworkRequest request = createRequest("/api/auth/profile");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    if (!fullName.isEmpty()) body["fullName"] = fullName;
    if (!affiliation.isEmpty()) body["affiliation"] = affiliation;

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    profileUpdateReply_ = networkManager_->put(request, data);

    connect(profileUpdateReply_, &QNetworkReply::finished, this, &AuthManager::onProfileUpdateReply);
}

void AuthManager::changePassword(const QString& oldPassword, const QString& newPassword) {

    QNetworkRequest request = createRequest("/api/auth/change-password");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["oldPassword"] = oldPassword;
    body["newPassword"] = newPassword;

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    passwordChangeReply_ = networkManager_->post(request, data);

    connect(passwordChangeReply_, &QNetworkReply::finished, this, &AuthManager::onPasswordChangeReply);
}

// ============================================================================
// State Management
// ============================================================================

void AuthManager::setTokens(const DesktopAuthTokens& tokens) {
    tokens_ = tokens;
    saveTokens();

    // Schedule auto-refresh
    if (!tokens_.accessToken.isEmpty()) {
        scheduleTokenRefresh();
    }
}

bool AuthManager::loadSavedState() {
    if (loadTokens()) {
        authenticated_ = true;
        emit authenticationChanged(true);
        scheduleTokenRefresh();
        return true;
    }
    return false;
}

void AuthManager::clearSavedState() {
    clearTokens();
    authenticated_ = false;
    user_ = DesktopUser();
    emit authenticationChanged(false);

    if (refreshTimer_->isActive()) {
        refreshTimer_->stop();
    }
}

// ============================================================================
// Private Slots
// ============================================================================

void AuthManager::onLoginReply() {
    if (!loginReply_) return;

    QNetworkReply* reply = loginReply_;
    reply->deleteLater();
    loginReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();

        if (response["success"].toBool()) {
            QJsonObject data = response["data"].toObject();

            // Parse user
            QJsonObject userJson = data["user"].toObject();
            user_ = parseUser(userJson);

            // Parse tokens
            QJsonObject tokensJson = data["tokens"].toObject();
            tokens_ = parseTokens(tokensJson);

            saveTokens();
            authenticated_ = true;

            emit authenticationChanged(true);
            emit loginSuccess(user_);

            scheduleTokenRefresh();
        } else {
            QString error = response["message"].toString();
            emit loginFailed(error);
        }
    } else {
        QString error = reply->errorString();
        emit loginFailed(error);
    }
}

void AuthManager::onRegisterReply() {
    if (!registerReply_) return;

    QNetworkReply* reply = registerReply_;
    reply->deleteLater();
    registerReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();

        if (response["success"].toBool()) {
            QJsonObject data = response["data"].toObject();

            // Parse user
            QJsonObject userJson = data["user"].toObject();
            user_ = parseUser(userJson);

            // Parse tokens
            QJsonObject tokensJson = data["tokens"].toObject();
            tokens_ = parseTokens(tokensJson);

            saveTokens();
            authenticated_ = true;

            emit authenticationChanged(true);
            emit registerSuccess(user_);

            scheduleTokenRefresh();
        } else {
            QString error = response["message"].toString();
            emit registerFailed(error);
        }
    } else {
        QString error = reply->errorString();
        emit registerFailed(error);
    }
}

void AuthManager::onLogoutReply() {
    if (!logoutReply_) return;

    QNetworkReply* reply = logoutReply_;
    reply->deleteLater();
    logoutReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        clearTokens();
        authenticated_ = false;
        user_ = DesktopUser();

        emit authenticationChanged(false);
        emit logoutSuccess();

        if (refreshTimer_->isActive()) {
            refreshTimer_->stop();
        }
    } else {
        QString error = reply->errorString();
        emit logoutFailed(error);
    }
}

void AuthManager::onRefreshReply() {
    if (!refreshReply_) return;

    QNetworkReply* reply = refreshReply_;
    reply->deleteLater();
    refreshReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();

        if (response["success"].toBool()) {
            QJsonObject data = response["data"].toObject();

            tokens_.accessToken = data["accessToken"].toString();
            tokens_.expiresAt = data["expiresAt"].toVariant().toLongLong();

            saveTokens();

            emit tokenRefreshed(tokens_.accessToken);
            scheduleTokenRefresh();
        } else {
            // Refresh failed - logout user
            QString error = response["message"].toString();
            emit tokenRefreshFailed(error);

            clearSavedState();
        }
    } else {
        QString error = reply->errorString();
        emit tokenRefreshFailed(error);

        // On refresh failure, logout user
        clearSavedState();
    }
}

void AuthManager::onUserFetchReply() {
    if (!userFetchReply_) return;

    QNetworkReply* reply = userFetchReply_;
    reply->deleteLater();
    userFetchReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();

        if (response["success"].toBool()) {
            QJsonObject data = response["data"].toObject();
            user_ = parseUser(data);

            emit userFetched(user_);
        } else {
            QString error = response["message"].toString();
            emit userFetchFailed(error);
        }
    } else {
        QString error = reply->errorString();
        emit userFetchFailed(error);
    }
}

void AuthManager::onProfileUpdateReply() {
    if (!profileUpdateReply_) return;

    QNetworkReply* reply = profileUpdateReply_;
    reply->deleteLater();
    profileUpdateReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject response = doc.object();

        if (response["success"].toBool()) {
            QJsonObject data = response["data"].toObject();
            user_ = parseUser(data);

            emit profileUpdated(user_);
        } else {
            QString error = response["message"].toString();
            emit profileUpdateFailed(error);
        }
    } else {
        QString error = reply->errorString();
        emit profileUpdateFailed(error);
    }
}

void AuthManager::onPasswordChangeReply() {
    if (!passwordChangeReply_) return;

    QNetworkReply* reply = passwordChangeReply_;
    reply->deleteLater();
    passwordChangeReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        emit passwordChanged();
    } else {
        QString error = reply->errorString();
        emit passwordChangeFailed(error);
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void AuthManager::saveTokens() {
    settings_->setValue("auth/tokens/access_token", tokens_.accessToken);
    settings_->setValue("auth/tokens/refresh_token", tokens_.refreshToken);
    settings_->setValue("auth/tokens/expires_at", tokens_.expiresAt);
}

bool AuthManager::loadTokens() {
    tokens_.accessToken = settings_->value("auth/tokens/access_token").toString();
    tokens_.refreshToken = settings_->value("auth/tokens/refresh_token").toString();
    tokens_.expiresAt = settings_->value("auth/tokens/expires_at").toLongLong();

    return !tokens_.accessToken.isEmpty() && !tokens_.refreshToken.isEmpty();
}

void AuthManager::clearTokens() {
    settings_->remove("auth/tokens");
    tokens_ = DesktopAuthTokens();
}

void AuthManager::scheduleTokenRefresh() {
    if (tokens_.expiresAt == 0) return;

    qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 refreshTime = tokens_.expiresAt - now - 300; // 5 minutes before expiry

    if (refreshTime > 0) {
        refreshTimer_->start(refreshTime * 1000); // Convert to milliseconds
    } else {
        // Token already expiring soon, refresh now
        refreshToken();
    }
}

QNetworkRequest AuthManager::createRequest(const QString& endpoint) {
    QNetworkRequest request;
    request.setUrl(baseUrl_ + endpoint);

    if (!tokens_.accessToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + tokens_.accessToken.toUtf8());
    }

    return request;
}

DesktopUser AuthManager::parseUser(const QJsonObject& json) {
    DesktopUser user;

    user.id = json["id"].toInt();
    user.username = json["username"].toString();
    user.email = json["email"].toString();
    user.fullName = json["fullName"].toString();
    user.role = json["role"].toString();
    user.isActive = json["isActive"].toBool();
    user.avatarUrl = json["avatarUrl"].toString();

    return user;
}

DesktopAuthTokens AuthManager::parseTokens(const QJsonObject& json) {
    DesktopAuthTokens tokens;

    tokens.accessToken = json["accessToken"].toString();
    tokens.refreshToken = json["refreshToken"].toString();
    tokens.expiresAt = json["expiresAt"].toVariant().toLongLong();

    return tokens;
}
