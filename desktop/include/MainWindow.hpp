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

// Forward declarations
class SearchWidget;
class ResultView;
class ProgressView;
class FilterPanel;
class ThemeManager;
class HeroWidget;
class FeatureCards;
class PaperCardView;
class ApiManager;
class QAction;
class QMenu;
class QToolBar;
class QCloseEvent;
class QPushButton;

// Forward declarations for API types
struct SearchResult;
struct ApiPaper;

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
    void onExport();
    void onPreferences();
    void onToggleTheme();
    void onShowStatistics();
    void onAbout();

    // API slots
    void onSearchSuccess(const SearchResult& result);
    void onSearchFailed(const QString& error);
    void onHealthCheckSuccess(bool healthy, const QString& message);
    void onNetworkError(const QString& error);

private:
    void setupUI();
    void createMenus();
    void createToolBar();
    void createThemeButton();
    void connectSignals();
    void loadSettings();
    void saveSettings();

    // UI Components
    HeroWidget* heroWidget_{nullptr};
    FeatureCards* featureCards_{nullptr};
    SearchWidget* searchWidget_{nullptr};
    PaperCardView* resultView_{nullptr};
    ResultView* tableView_{nullptr};
    ProgressView* progressView_{nullptr};
    FilterPanel* filterPanel_{nullptr};
    QPointer<ThemeManager> themeManager_;

    // API Manager
    ApiManager* apiManager_{nullptr};

    // Actions
    QAction* searchAction_{nullptr};
    QAction* exportAction_{nullptr};
    QAction* settingsAction_{nullptr};
    QAction* aboutAction_{nullptr};
    QAction* themeAction_{nullptr};
    QPushButton* themeButton_{nullptr};

    // State
    bool darkMode_{false};
};
