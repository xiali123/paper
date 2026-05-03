#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QProgressBar>
#include <QElapsedTimer>
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
class LatexEditorWidget;
class ReadingListManager;
class AdvancedSearchDialog;
class OfflineCacheManager;
class RecentHistoryWidget;
class BatchOperationsBar;
class SystemTrayManager;
class WelcomeWidget;
class NotificationCenter;
class DragDropHandler;
class CommandPalette;
class SessionManager;
class UpdateChecker;
class SearchSuggestWidget;
class PaperExportDialog;
class ThemeCustomizer;
class FilterChipBar;
class PaperStatsChart;
class QuickNoteWidget;
class PaperCollectionWidget;
class SideBySideDiff;
class ProgressTracker;
class BackupRestoreWidget;
class AdvancedTableWidget;
class PaperRecommendationEngine;
class PdfThumbnailWidget;

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
    void populateRecommendations(const QJsonObject& data);
    void populateAdminDashboard(const QJsonObject& data);
    void populateAdminUsers(const QJsonObject& data);
    void populateAdminModules(const QJsonObject& data);
    void populateAdminMonitor(const QJsonObject& data);
    void populatePerformanceMetrics(const QJsonObject& data);
    void showLoginHistory(const QJsonObject& data);
    void populateCrawlerSchedules(const QJsonObject& data);
    void populateCrawlerTemplates(const QJsonObject& data);
    void populateSavedSearches(const QJsonObject& data);
    void populateTrendingSearches(const QJsonObject& data);
    void showStatsDialog(const QString& title, const QJsonObject& data);
    void showAiStatusDialog(const QJsonObject& data);
    void showCrawlerWorkersDialog(const QJsonObject& data);
    void showCrawlerStatisticsDialog(const QJsonObject& data);
    void showSearchHistoryDialog(const QJsonObject& data);
    void showAllPapersDialog(const QJsonObject& data);
    void showRecommendationExplanationDialog(const QJsonObject& data);
    void showLatexTemplatesDialog(const QJsonObject& data);

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
    LatexEditorWidget* latexEditor_{nullptr};
    ReadingListManager* readingListMgr_{nullptr};
    OfflineCacheManager* cacheManager_{nullptr};
    RecentHistoryWidget* recentHistory_{nullptr};
    BatchOperationsBar* batchBar_{nullptr};
    SystemTrayManager* trayManager_{nullptr};
    WelcomeWidget* welcomeWidget_{nullptr};
    NotificationCenter* notificationCenter_{nullptr};
    CommandPalette* commandPalette_{nullptr};
    SessionManager* sessionManager_{nullptr};
    UpdateChecker* updateChecker_{nullptr};
    SearchSuggestWidget* searchSuggest_{nullptr};
    ThemeCustomizer* themeCustomizer_{nullptr};
    FilterChipBar* filterChipBar_{nullptr};
    QuickNoteWidget* quickNotes_{nullptr};
    PaperCollectionWidget* collections_{nullptr};
    ProgressTracker* progressTracker_{nullptr};
    int lastRunningCount_{0};
    QElapsedTimer healthTimer_;

    // State
    bool darkMode_{false};

    // Search & Pagination state
    QString currentKeyword_;
    int currentOffset_{0};
    int currentLimit_{20};
    int totalResults_{0};
};
