/**
 * Login Window Implementation
 *
 * Qt/C++ implementation for login/registration dialog
 */

#include "LoginWindow.hpp"
#include "AuthManager.hpp"
#include "ModernButton.hpp"
#include "ModernCard.hpp"
#include "ThemeManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QStackedWidget>
#include <QMessageBox>
#include <QTimer>

LoginWindow::LoginWindow(AuthManager* authManager, QWidget* parent)
    : QDialog(parent)
    , authManager_(authManager)
    , isRegisterMode_(false)
    , loginPasswordVisible_(false)
    , registerPasswordVisible_(false)
    , confirmPasswordVisible_(false)
{
    setWindowTitle("PaperCrawler - Login");
    setModal(true);
    setMinimumWidth(500);
    setMinimumHeight(400);

    setupUI();
    applyTheme();

    // Connect auth manager signals
    connect(authManager_, &AuthManager::loginSuccess,
            this, &LoginWindow::onLoginSuccess);
    connect(authManager_, &AuthManager::loginFailed,
            this, &LoginWindow::onLoginFailed);
    connect(authManager_, &AuthManager::registerSuccess,
            this, &LoginWindow::onRegisterSuccess);
    connect(authManager_, &AuthManager::registerFailed,
            this, &LoginWindow::onRegisterFailed);
}

LoginWindow::~LoginWindow() {
    // Cleanup handled by Qt
}

// ============================================================================
// Action Slots
// ============================================================================

void LoginWindow::onLoginClicked() {
    if (!validateLoginForm()) {
        return;
    }

    QString email = emailEdit_->text().trimmed();
    QString password = passwordEdit_->text();

    // Disable buttons
    loginButton_->setEnabled(false);
    updateErrorMessage("", false);

    // Attempt login
    authManager_->login(email, password);
}

void LoginWindow::onRegisterClicked() {
    if (!validateRegisterForm()) {
        return;
    }

    QString username = usernameEdit_->text().trimmed();
    QString email = registerEmailEdit_->text().trimmed();
    QString password = registerPasswordEdit_->text();
    QString fullName = fullNameEdit_->text().trimmed();

    // Disable buttons
    registerButton_->setEnabled(false);
    updateErrorMessage("", false);

    // Attempt registration
    authManager_->registerUser(username, email, password, fullName);
}

void LoginWindow::onLoginSuccess(const DesktopUser& user) {
    updateErrorMessage("", false);
    accept();
    emit authenticationSuccessful(user);
}

void LoginWindow::onLoginFailed(const QString& error) {
    loginButton_->setEnabled(true);
    updateErrorMessage(error, true);
}

void LoginWindow::onRegisterSuccess(const DesktopUser& user) {
    updateErrorMessage("", false);
    accept();
    emit registrationCompleted(user);
}

void LoginWindow::onRegisterFailed(const QString& error) {
    registerButton_->setEnabled(true);
    updateErrorMessage(error, true);
}

void LoginWindow::onTogglePasswordVisibility() {
    loginPasswordVisible_ = !loginPasswordVisible_;
    passwordEdit_->setEchoMode(loginPasswordVisible_ ? QLineEdit::Normal : QLineEdit::Password);

    // Update button text
    togglePasswordButton_->setText(loginPasswordVisible_ ? "Hide" : "Show");
}

void LoginWindow::switchToRegisterMode() {
    isRegisterMode_ = true;
    stackedWidget_->setCurrentWidget(registerWidget_);
    clearForms();
}

void LoginWindow::switchToLoginMode() {
    isRegisterMode_ = false;
    stackedWidget_->setCurrentWidget(loginWidget_);
    clearForms();
}

// ============================================================================
// UI Setup
// ============================================================================

void LoginWindow::setupUI() {
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setContentsMargins(20, 20, 20, 20);
    mainLayout_->setSpacing(20);

    // Create stacked widget for login/register forms
    stackedWidget_ = new QStackedWidget(this);

    // Setup forms
    setupLoginUI();
    setupRegisterUI();

    stackedWidget_->addWidget(loginWidget_);
    stackedWidget_->addWidget(registerWidget_);
    stackedWidget_->setCurrentWidget(loginWidget_);

    mainLayout_->addWidget(stackedWidget_);

    setLayout(mainLayout_);
}

void LoginWindow::setupLoginUI() {
    loginWidget_ = new QWidget();
    loginLayout_ = new QVBoxLayout(loginWidget_);
    loginLayout_->setSpacing(15);

    // Title
    loginTitleLabel_ = new QLabel("Welcome to PaperCrawler");
    loginTitleLabel_->setAlignment(Qt::AlignCenter);
    loginTitleLabel_->setStyleSheet("font-size: 20px; font-weight: bold; margin-bottom: 10px;");

    // Email
    QLabel* emailLabel = new QLabel("Email:");
    emailEdit_ = new QLineEdit();
    emailEdit_->setPlaceholderText("your.email@example.com");

    // Password
    QLabel* passwordLabel = new QLabel("Password:");
    passwordEdit_ = new QLineEdit();
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setPlaceholderText("Enter your password");

    QHBoxLayout* passwordLayout = new QHBoxLayout();
    passwordLayout->addWidget(passwordEdit_);

    togglePasswordButton_ = new QPushButton("Show");
    togglePasswordButton_->setMaximumWidth(80);
    connect(togglePasswordButton_, &QPushButton::clicked,
            this, &LoginWindow::onTogglePasswordVisibility);
    passwordLayout->addWidget(togglePasswordButton_);

    // Remember me
    rememberMeCheckbox_ = new QCheckBox("Remember me");

    // Error message
    loginErrorLabel_ = new QLabel();
    loginErrorLabel_->setWordWrap(true);
    loginErrorLabel_->setStyleSheet("color: red; padding: 10px;");

    // Login button
    loginButton_ = new QPushButton("Sign In");
    loginButton_->setMinimumHeight(40);
    connect(loginButton_, &QPushButton::clicked,
            this, &LoginWindow::onLoginClicked);

    // Switch to register
    switchToRegisterButton_ = new QPushButton("Create an account");
    switchToRegisterButton_->setFlat(true);
    connect(switchToRegisterButton_, &QPushButton::clicked,
            this, &LoginWindow::switchToRegisterMode);

    // Add to layout
    loginLayout_->addWidget(loginTitleLabel_);
    loginLayout_->addWidget(emailLabel);
    loginLayout_->addWidget(emailEdit_);
    loginLayout_->addWidget(passwordLabel);
    loginLayout_->addLayout(passwordLayout);
    loginLayout_->addWidget(rememberMeCheckbox_);
    loginLayout_->addWidget(loginErrorLabel_);
    loginLayout_->addWidget(loginButton_);
    loginLayout_->addWidget(switchToRegisterButton_);
    loginLayout_->addStretch();
}

void LoginWindow::setupRegisterUI() {
    registerWidget_ = new QWidget();
    registerLayout_ = new QVBoxLayout(registerWidget_);
    registerLayout_->setSpacing(10);

    // Title
    registerTitleLabel_ = new QLabel("Create Account");
    registerTitleLabel_->setAlignment(Qt::AlignCenter);
    registerTitleLabel_->setStyleSheet("font-size: 20px; font-weight: bold; margin-bottom: 10px;");

    // Username
    QLabel* usernameLabel = new QLabel("Username:");
    usernameEdit_ = new QLineEdit();
    usernameEdit_->setPlaceholderText("Choose a username");

    // Email
    QLabel* emailLabel = new QLabel("Email:");
    registerEmailEdit_ = new QLineEdit();
    registerEmailEdit_->setPlaceholderText("your.email@example.com");

    // Full Name
    QLabel* fullNameLabel = new QLabel("Full Name:");
    fullNameEdit_ = new QLineEdit();
    fullNameEdit_->setPlaceholderText("Your full name");

    // Password
    QLabel* passwordLabel = new QLabel("Password:");
    registerPasswordEdit_ = new QLineEdit();
    registerPasswordEdit_->setEchoMode(QLineEdit::Password);
    registerPasswordEdit_->setPlaceholderText("Create a strong password");

    // Confirm Password
    QLabel* confirmLabel = new QLabel("Confirm Password:");
    confirmPasswordEdit_ = new QLineEdit();
    confirmPasswordEdit_->setEchoMode(QLineEdit::Password);
    confirmPasswordEdit_->setPlaceholderText("Confirm your password");

    // Error message
    registerErrorLabel_ = new QLabel();
    registerErrorLabel_->setWordWrap(true);
    registerErrorLabel_->setStyleSheet("color: red; padding: 10px;");

    // Register button
    registerButton_ = new QPushButton("Create Account");
    registerButton_->setMinimumHeight(40);
    connect(registerButton_, &QPushButton::clicked,
            this, &LoginWindow::onRegisterClicked);

    // Switch to login
    switchToLoginButton_ = new QPushButton("Already have an account? Sign in");
    switchToLoginButton_->setFlat(true);
    connect(switchToLoginButton_, &QPushButton::clicked,
            this, &LoginWindow::switchToLoginMode);

    // Add to layout
    registerLayout_->addWidget(registerTitleLabel_);
    registerLayout_->addWidget(usernameLabel);
    registerLayout_->addWidget(usernameEdit_);
    registerLayout_->addWidget(emailLabel);
    registerLayout_->addWidget(registerEmailEdit_);
    registerLayout_->addWidget(fullNameLabel);
    registerLayout_->addWidget(fullNameEdit_);
    registerLayout_->addWidget(passwordLabel);
    registerLayout_->addWidget(registerPasswordEdit_);
    registerLayout_->addWidget(confirmLabel);
    registerLayout_->addWidget(confirmPasswordEdit_);
    registerLayout_->addWidget(registerErrorLabel_);
    registerLayout_->addWidget(registerButton_);
    registerLayout_->addWidget(switchToLoginButton_);
    registerLayout_->addStretch();
}

void LoginWindow::applyTheme() {
    // Get theme from ThemeManager
    ThemeManager* themeManager = ThemeManager::instance();
    bool isDark = themeManager->isDarkMode();

    QString backgroundColor = isDark ? "#2d2d2d" : "#ffffff";
    QString textColor = isDark ? "#ffffff" : "#000000";
    QString borderColor = isDark ? "#444444" : "#cccccc";

    setStyleSheet(QString(
        "QDialog { background-color: %1; }"
        "QLabel { color: %2; }"
        "QLineEdit {"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  background-color: %4;"
        "  color: %2;"
        "}"
        "QPushButton {"
        "  background-color: #667eea;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 10px;"
        "}"
        "QPushButton:hover { background-color: #5568d3; }"
        "QPushButton:pressed { background-color: #4c5db8; }"
        "QPushButton:disabled { background-color: #cccccc; }"
    ).arg(backgroundColor).arg(textColor).arg(borderColor)
     .arg(isDark ? "#3d3d3d" : "#f5f5f5"));
}

void LoginWindow::updateErrorMessage(const QString& message, bool isError) {
    if (isRegisterMode_) {
        if (message.isEmpty()) {
            registerErrorLabel_->clear();
            registerErrorLabel_->hide();
        } else {
            registerErrorLabel_->setText(message);
            registerErrorLabel_->setStyleSheet(isError ? "color: red; padding: 10px;" : "color: green; padding: 10px;");
            registerErrorLabel_->show();
        }
    } else {
        if (message.isEmpty()) {
            loginErrorLabel_->clear();
            loginErrorLabel_->hide();
        } else {
            loginErrorLabel_->setText(message);
            loginErrorLabel_->setStyleSheet(isError ? "color: red; padding: 10px;" : "color: green; padding: 10px;");
            loginErrorLabel_->show();
        }
    }
}

void LoginWindow::clearForms() {
    // Clear login form
    emailEdit_->clear();
    passwordEdit_->clear();

    // Clear register form
    usernameEdit_->clear();
    registerEmailEdit_->clear();
    fullNameEdit_->clear();
    registerPasswordEdit_->clear();
    confirmPasswordEdit_->clear();

    // Clear errors
    updateErrorMessage("", false);
}

bool LoginWindow::validateLoginForm() {
    QString email = emailEdit_->text().trimmed();
    QString password = passwordEdit_->text();

    if (email.isEmpty()) {
        updateErrorMessage("Please enter your email", true);
        return false;
    }

    if (!email.contains('@') || !email.contains('.')) {
        updateErrorMessage("Please enter a valid email address", true);
        return false;
    }

    if (password.isEmpty()) {
        updateErrorMessage("Please enter your password", true);
        return false;
    }

    if (password.length() < 8) {
        updateErrorMessage("Password must be at least 8 characters", true);
        return false;
    }

    return true;
}

bool LoginWindow::validateRegisterForm() {
    QString username = usernameEdit_->text().trimmed();
    QString email = registerEmailEdit_->text().trimmed();
    QString fullName = fullNameEdit_->text().trimmed();
    QString password = registerPasswordEdit_->text();
    QString confirmPassword = confirmPasswordEdit_->text();

    if (username.isEmpty()) {
        updateErrorMessage("Please enter a username", true);
        return false;
    }

    if (username.length() < 3) {
        updateErrorMessage("Username must be at least 3 characters", true);
        return false;
    }

    if (email.isEmpty()) {
        updateErrorMessage("Please enter your email", true);
        return false;
    }

    if (!email.contains('@') || !email.contains('.')) {
        updateErrorMessage("Please enter a valid email address", true);
        return false;
    }

    if (password.isEmpty()) {
        updateErrorMessage("Please enter a password", true);
        return false;
    }

    if (password.length() < 8) {
        updateErrorMessage("Password must be at least 8 characters", true);
        return false;
    }

    if (confirmPassword.isEmpty()) {
        updateErrorMessage("Please confirm your password", true);
        return false;
    }

    if (password != confirmPassword) {
        updateErrorMessage("Passwords do not match", true);
        return false;
    }

    return true;
}
