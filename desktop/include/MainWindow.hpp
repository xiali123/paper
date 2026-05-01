#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QProgressBar>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <QTabWidget>
#include <memory>
#include <QPointer>
#include <QSystemTrayIcon>
#include "PaperTypes.hpp"
#include "PaperCardView.hpp"

class SearchWidget;
class ResultView;
class ProgressView;
class FilterPanel;
class ThemeManager;
class HeroWidget;
class FeatureCards;
class ApiManager;
class PaperCache;
class LocalDatabase;
class ExportManager;
class AuthManager;
class LoginWindow;
class QAction;
class QMenu;
class QToolBar;
class QCloseEvent;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onSearch(const QString& keyword);
    void onPaperSelected(int paperId);
    void onExport(ExportFormat format = ExportFormat::CSV);
    void onPreferences();
    void onToggleTheme();
    void onShowStatistics();
    void onAbout();

    // API slots
    void onSearchSuccess(const SearchResult& result);
    void onSearchFailed(const QString& error);
    void onHealthCheckSuccess(bool healthy, const QString& message);
    void onNetworkError(const QString& error);
    void onPaperDetailsSuccess(const Paper& paper);

    // Pagination slot
    void onPageChanged(int offset, int limit);

    // Auth slots
    void onLoginSuccess(const DesktopUser& user);
    void onLogoutSuccess();
    void onAuthenticationChanged(bool authenticated);
    void showLoginDialog();
    void handleLogout();
    void updateAuthUI();

    // Database slots
    void onPaperAdded(int paperId);
    void onDatabaseError(const QString& error);

private:
    void setupUI();
    void createMenus();
    void createToolBar();
    void createThemeButton();
    void connectSignals();
    void loadSettings();
    void saveSettings();
    void initializeAuthentication();
    void refreshFavoritesTab();

    // UI Components
    QTabWidget* tabWidget_{nullptr};
    HeroWidget* heroWidget_{nullptr};
    FeatureCards* featureCards_{nullptr};
    SearchWidget* searchWidget_{nullptr};
    PaperCardView* resultView_{nullptr};
    ResultView* tableView_{nullptr};
    ProgressView* progressView_{nullptr};
    FilterPanel* filterPanel_{nullptr};

    // Core managers
    ThemeManager* themeManager_{nullptr};
    ApiManager* apiManager_{nullptr};
    PaperCache* paperCache_{nullptr};
    LocalDatabase* localDb_{nullptr};
    ExportManager* exportManager_{nullptr};

    // Auth
    AuthManager* authManager_{nullptr};
    LoginWindow* loginWindow_{nullptr};
    QAction* loginAction_{nullptr};
    QAction* logoutAction_{nullptr};
    QAction* profileAction_{nullptr};
    QLabel* userLabel_{nullptr};

    // Actions
    QAction* searchAction_{nullptr};
    QAction* exportAction_{nullptr};
    QAction* settingsAction_{nullptr};
    QAction* aboutAction_{nullptr};
    QAction* themeAction_{nullptr};
    QPushButton* themeButton_{nullptr};
    QSystemTrayIcon* trayIcon_{nullptr};
    int lastRunningCount_{0};

    // State
    bool darkMode_{false};

    // Search & Pagination state
    QString currentKeyword_;
    int currentOffset_{0};
    int currentLimit_{20};
    int totalResults_{0};
};
