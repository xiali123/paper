/**
 * MainWindow Authentication Integration
 *
 * This file shows how to integrate authentication into MainWindow
 * Add these sections to your MainWindow.cpp file
 */

#include "AuthManager.hpp"
#include "LoginWindow.hpp"
#include <QAction>
#include <QToolBar>

namespace PaperCrawler {

// ============================================================================
// Step 1: Add AuthManager member to MainWindow class
// ============================================================================

// In MainWindow.hpp, add to private members:
// AuthManager* authManager_{nullptr};
// LoginWindow* loginWindow_{nullptr};
// QAction* loginAction_{nullptr};
// QAction* logoutAction_{nullptr};
// QAction* profileAction_{nullptr};
// QLabel* userLabel_{nullptr};

// ============================================================================
// Step 2: Initialize AuthManager in MainWindow constructor
// ============================================================================

void MainWindow::initializeAuthentication() {
    // Create auth manager
    authManager_ = new AuthManager(this);

    // Configure
    authManager_->setBaseUrl("http://localhost:8080");
    authManager_->setApiManager(apiManager_); // Your existing API manager

    // Connect signals
    connect(authManager_, &AuthManager::authenticationChanged,
            this, &MainWindow::onAuthenticationChanged);
    connect(authManager_, &AuthManager::loginSuccess,
            this, &MainWindow::onLoginSuccess);
    connect(authManager_, &AuthManager->logoutSuccess,
            this, &MainWindow::onLogoutSuccess);

    // Try to load saved session
    if (authManager_->loadSavedState()) {
        // User already logged in
        qDebug() << "Restored session for user:" << authManager_->getCurrentUser().username;
    }

    // Update UI
    updateAuthUI();
}

// ============================================================================
// Step 3: Create authentication UI elements
// ============================================================================

void MainWindow::createAuthUI() {
    // Create toolbar actions
    loginAction_ = new QAction("Login", this);
    logoutAction_ = new QAction("Logout", this);
    profileAction_ = new QAction("Profile", this);

    // Connect actions
    connect(loginAction_, &QAction::triggered, this, &MainWindow::showLoginDialog);
    connect(logoutAction_, &QAction::triggered, this, &MainWindow::handleLogout);
    connect(profileAction_, &QAction::triggered, this, &MainWindow::showProfileDialog);

    // Add to toolbar
    toolBar_->addAction(loginAction_);
    toolBar_->addAction(profileAction_);
    toolBar_->addAction(logoutAction_);

    // Create user label in status bar
    userLabel_ = new QLabel(statusBar());
    statusBar()->addPermanentWidget(userLabel_);

    // Initial UI state
    updateAuthUI();
}

// ============================================================================
// Step 4: Show login dialog
// ============================================================================

void MainWindow::showLoginDialog() {
    if (!authManager_) {
        QMessageBox::critical(this, "Error", "Authentication system not initialized");
        return;
    }

    // Create and show login window
    loginWindow_ = new LoginWindow(authManager_, this);

    connect(loginWindow_, &LoginWindow::authenticationSuccessful,
            this, [this](const DesktopUser& user) {
                QMessageBox::information(this, "Welcome",
                    QString("Welcome, %1!").arg(user.fullName));
                onLoginSuccess(user);
            });

    loginWindow_->exec();
}

// ============================================================================
// Step 5: Handle authentication state changes
// ============================================================================

void MainWindow::onAuthenticationChanged(bool authenticated) {
    qDebug() << "Authentication state changed:" << authenticated;

    if (authenticated) {
        DesktopUser user = authManager_->getCurrentUser();
        qDebug() << "User logged in:" << user.username;
    } else {
        qDebug() << "User logged out";
    }

    updateAuthUI();
}

void MainWindow::onLoginSuccess(const DesktopUser& user) {
    qDebug() << "Login successful for:" << user.username;

    // Refresh data that might depend on authentication
    refreshPapers();
    refreshStats();
}

void MainWindow::onLogoutSuccess() {
    qDebug() << "Logout successful";

    // Clear user-specific data
    clearUserData();
}

void MainWindow::handleLogout() {
    if (!authManager_) return;

    // Confirm logout
    auto reply = QMessageBox::question(this, "Logout",
        "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        authManager_->logout();
    }
}

// ============================================================================
// Step 6: Update UI based on authentication state
// ============================================================================

void MainWindow::updateAuthUI() {
    if (!authManager_) return;

    bool authenticated = authManager_->isAuthenticated();

    // Show/hide actions
    loginAction_->setVisible(!authenticated);
    logoutAction_->setVisible(authenticated);
    profileAction_->setVisible(authenticated);

    // Update user label
    if (authenticated) {
        DesktopUser user = authManager_->getCurrentUser();
        userLabel_->setText(QString("👤 %1").arg(user.username));
        userLabel_->setToolTip(QString("Logged in as %1\n%2")
            .arg(user.fullName)
            .arg(user.email));
    } else {
        userLabel_->setText("");
        userLabel_->setToolTip("");
    }

    // Enable/disable features that require authentication
    searchWidget_->setEnabled(authenticated);
    resultView_->setEnabled(authenticated);

    // Update window title
    if (authenticated) {
        DesktopUser user = authManager_->getCurrentUser();
        setWindowTitle(QString("PaperCrawler - %1").arg(user.username));
    } else {
        setWindowTitle("PaperCrawler - Please Login");
    }
}

// ============================================================================
// Step 7: Show profile dialog
// ============================================================================

void MainWindow::showProfileDialog() {
    if (!authManager_ || !authManager_->isAuthenticated()) {
        return;
    }

    DesktopUser user = authManager_->getCurrentUser();

    QString info = QString(
        "Username: %1\n"
        "Email: %2\n"
        "Full Name: %3\n"
        "Role: %4\n"
    ).arg(user.username)
     .arg(user.email)
     .arg(user.fullName)
     .arg(user.role);

    QMessageBox::information(this, "My Profile", info);
}

// ============================================================================
// Step 8: Update existing API calls to include authentication
// ============================================================================

void MainWindow::searchPapers(const QString& query) {
    if (!authManager_ || !authManager_->isAuthenticated()) {
        QMessageBox::warning(this, "Authentication Required",
            "Please login to search papers");
        showLoginDialog();
        return;
    }

    // Get access token
    QString accessToken = authManager_->getAccessToken();

    // Add to API request
    QNetworkRequest request;
    request.setUrl(apiUrl_ + "/api/search?query=" + query);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    // ... existing search logic ...
}

void MainWindow::loadPaperDetails(int paperId) {
    if (!authManager_ || !authManager_->isAuthenticated()) {
        return;
    }

    // Get access token
    QString accessToken = authManager_->getAccessToken();

    // Add to API request
    QNetworkRequest request;
    request.setUrl(apiUrl_ + "/api/papers/" + QString::number(paperId));
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    // ... existing load logic ...
}

// ============================================================================
// Step 9: Handle token refresh (automatic)
// ============================================================================

// The AuthManager automatically handles token refresh
// Just make sure your API requests use the current access token:

void MainWindow::makeAuthenticatedRequest(const QString& endpoint) {
    if (!authManager_ || !authManager_->isAuthenticated()) {
        return;
    }

    QNetworkRequest request;
    request.setUrl(apiUrl_ + endpoint);

    // Always get fresh access token
    QString accessToken = authManager_->getAccessToken();
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());

    // Make request
    QNetworkReply* reply = networkManager_->get(request);

    // Handle 401 responses (token expired)
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError error) {
        if (error == QNetworkReply::AuthenticationRequiredError) {
            // Token might have expired, AuthManager will auto-refresh
            // Retry the request after a short delay
            QTimer::singleShot(1000, this, [this]() {
                // Retry logic here
            });
        }
    });
}

// ============================================================================
// Step 10: Cleanup on exit
// ============================================================================

void MainWindow::closeEvent(QCloseEvent* event) {
    // Optionally logout on exit (uncomment if desired)
    // if (authManager_ && authManager_->isAuthenticated()) {
    //     authManager_->logout();
    // }

    QMainWindow::closeEvent(event);
}

} // namespace PaperCrawler

// ============================================================================
// Integration Checklist
// ============================================================================
/*
 * To integrate authentication into MainWindow:
 *
 * 1. Add members:
 *    - AuthManager* authManager_
 *    - LoginWindow* loginWindow_
 *    - QActions for login/logout/profile
 *    - QLabel for user display
 *
 * 2. In constructor:
 *    - Call initializeAuthentication()
 *    - Call createAuthUI()
 *
 * 3. Update all API calls:
 *    - Check authentication before making requests
 *    - Add Authorization header with access token
 *    - Handle 401 responses
 *
 * 4. Update UI:
 *    - Disable features for unauthenticated users
 *    - Show login dialog when needed
 *    - Display user info when authenticated
 *
 * 5. Test:
 *    - Login flow
 *    - Token refresh
 *    - Logout flow
 *    - Session persistence
 */
