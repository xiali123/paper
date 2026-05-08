#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

// Forward declarations
class AuthManager;
class ModernButton;
class ModernCard;

/**
 * @brief Login dialog window for desktop client
 *
 * Modern Qt6 dialog for user authentication
 * Supports login and registration modes
 * Integrates with ThemeManager for dark/light support
 *
 * Features:
 * - Login mode
 * - Registration mode
 * - Form validation
 * - Password visibility toggle
 * - Error display
 * - Theme-aware styling
 * - Modern card-based UI
 *
 * Usage:
 * @code
 * auto* authManager = new AuthManager(this);
 * auto* loginWindow = new LoginWindow(authManager, this);
 *
 * connect(loginWindow, &LoginWindow::authenticationSuccessful,
 *         this, &MainWindow::onAuthenticationSuccessful);
 *
 * loginWindow->exec();
 * @endcode
 */
class LoginWindow : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param authManager Authentication manager instance
     * @param parent Parent widget
     */
    explicit LoginWindow(AuthManager* authManager, QWidget* parent = nullptr);

    /**
     * @brief Destructor
     */
    ~LoginWindow();

signals:
    /**
     * @brief Emitted when authentication is successful
     * @param user Authenticated user
     */
    void authenticationSuccessful(const DesktopUser& user);

    /**
     * @brief Emitted when registration is completed
     * @param user Registered user
     */
    void registrationCompleted(const DesktopUser& user);

private slots:
    // ========================================================================
    // Action Slots
    // ========================================================================

    /**
     * @brief Handle login button click
     */
    void onLoginClicked();

    /**
     * @brief Handle register button click
     */
    void onRegisterClicked();

    /**
     * @brief Handle successful login
     * @param user Logged in user
     */
    void onLoginSuccess(const DesktopUser& user);

    /**
     * @brief Handle failed login
     * @param error Error message
     */
    void onLoginFailed(const QString& error);

    /**
     * @brief Handle successful registration
     * @param user Registered user
     */
    void onRegisterSuccess(const DesktopUser& user);

    /**
     * @brief Handle failed registration
     * @param error Error message
     */
    void onRegisterFailed(const QString& error);

    /**
     * @brief Toggle password visibility
     */
    void onTogglePasswordVisibility();

    /**
     * @brief Switch to registration mode
     */
    void switchToRegisterMode();

    /**
     * @brief Switch to login mode
     */
    void switchToLoginMode();

private:
    // ========================================================================
    // UI Setup
    // ========================================================================

    /**
     * @brief Setup UI components
     */
    void setupUI();

    /**
     * @brief Setup login form UI
     */
    void setupLoginUI();

    /**
     * @brief Setup registration form UI
     */
    void setupRegisterUI();

    /**
     * @brief Apply theme styling
     */
    void applyTheme();

    /**
     * @brief Update error message display
     * @param message Error message (empty to clear)
     * @param isError true if error, false if info
     */
    void updateErrorMessage(const QString& message, bool isError = true);

    /**
     * @brief Clear all form fields
     */
    void clearForms();

    /**
     * @brief Validate login form
     * @return true if valid
     */
    bool validateLoginForm();

    /**
     * @brief Validate registration form
     * @return true if valid
     */
    bool validateRegisterForm();

    // ========================================================================
    // UI Components
    // ========================================================================

    // Main container
    QVBoxLayout* mainLayout_;
    QHBoxLayout* contentLayout_;

    // Left side - branding/image
    QWidget* leftWidget_;
    QVBoxLayout* leftLayout_;
    QLabel* logoLabel_;
    QLabel* titleLabel_;
    QLabel* subtitleLabel_;

    // Right side - forms
    QStackedWidget* stackedWidget_;

    // Login form
    QWidget* loginWidget_;
    QVBoxLayout* loginLayout_;
    QLabel* loginTitleLabel_;
    QLineEdit* emailEdit_;
    QLineEdit* passwordEdit_;
    QPushButton* togglePasswordButton_;
    QCheckBox* rememberMeCheckbox_;
    QPushButton* loginButton_;
    QPushButton* switchToRegisterButton_;
    QLabel* loginErrorLabel_;

    // Register form
    QWidget* registerWidget_;
    QVBoxLayout* registerLayout_;
    QLabel* registerTitleLabel_;
    QLineEdit* usernameEdit_;
    QLineEdit* registerEmailEdit_;
    QLineEdit* fullNameEdit_;
    QLineEdit* registerPasswordEdit_;
    QLineEdit* confirmPasswordEdit_;
    QPushButton* toggleRegisterPasswordButton_;
    QPushButton* toggleConfirmPasswordButton_;
    QPushButton* registerButton_;
    QPushButton* switchToLoginButton_;
    QLabel* registerErrorLabel_;

    // State
    bool isRegisterMode_{false};
    AuthManager* authManager_;

    // Password visibility states
    bool loginPasswordVisible_{false};
    bool registerPasswordVisible_{false};
    bool confirmPasswordVisible_{false};
};
