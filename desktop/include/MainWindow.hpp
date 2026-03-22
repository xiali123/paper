#pragma once

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QProgressBar>
#include <QLabel>
#include <QTableView>
#include <QStandardItemModel>
#include <memory>
#include <QPointer>
#include "PaperCardView.hpp"

// Forward declarations
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
class QAction;
class QMenu;
class QToolBar;
class QCloseEvent;
class QPushButton;

// Forward declarations for API types
struct SearchResult;
struct ApiPaper;

// Forward declarations for database types
struct DbPaper;
struct DbSearchResult;

// Export format enum
enum class ExportFormat {
    CSV,
    BibTeX,
    JSON,
    PDF
};

// Forward declarations for API types
struct SearchResult;
struct ApiPaper;

// Forward declarations for database types
struct DbPaper;
struct DbSearchResult;

/**
 * @brief Main application window for PaperCrawler desktop client
 *
 * Modern Qt6 UI with:
 * - Gradient backgrounds
 * - Theme manager (light/dark mode)
 * - Glass morphism effects
 * - Smooth animations
 * - Real API integration
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

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

    // Pagination slot
    void onPageChanged(int offset, int limit);

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

    // UI Components (children of MainWindow, auto-deleted)
    HeroWidget* heroWidget_{nullptr};
    FeatureCards* featureCards_{nullptr};
    SearchWidget* searchWidget_{nullptr};
    PaperCardView* resultView_{nullptr};
    ResultView* tableView_{nullptr};
    ProgressView* progressView_{nullptr};
    FilterPanel* filterPanel_{nullptr};

    // Core managers (explicitly deleted in destructor)
    ThemeManager* themeManager_{nullptr};
    ApiManager* apiManager_{nullptr};
    PaperCache* paperCache_{nullptr};
    LocalDatabase* localDb_{nullptr};
    ExportManager* exportManager_{nullptr};

    // Actions
    QAction* searchAction_{nullptr};
    QAction* exportAction_{nullptr};
    QAction* settingsAction_{nullptr};
    QAction* aboutAction_{nullptr};
    QAction* themeAction_{nullptr};
    QPushButton* themeButton_{nullptr};

    // State
    bool darkMode_{false};

    // Search & Pagination state
    QString currentKeyword_;
    int currentOffset_{0};
    int currentLimit_{20};
    int totalResults_{0};
};
