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

// Forward declarations
class SearchWidget;
class ResultView;
class ProgressView;
class FilterPanel;
class QAction;
class QMenu;
class QToolBar;
class QCloseEvent;

/**
 * @brief Main application window for PaperCrawler desktop client
 *
 * Provides comprehensive GUI for paper searching, viewing, and managing
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onSearch(const QString& keyword);
    void onPaperSelected(int paperId);
    void onExport();
    void onPreferences();
    void onToggleTheme();
    void onShowStatistics();
    void onAbout();

private:
    void setupUI();
    void createMenus();
    void createToolBar();
    void connectSignals();
    void loadSettings();
    void saveSettings();

    // UI Components
    SearchWidget* searchWidget_{nullptr};
    ResultView* resultView_{nullptr};
    ProgressView* progressView_{nullptr};
    FilterPanel* filterPanel_{nullptr};

    // Actions
    QAction* searchAction_{nullptr};
    QAction* exportAction_{nullptr};
    QAction* settingsAction_{nullptr};
    QAction* aboutAction_{nullptr};
    QAction* themeAction_{nullptr};

    // State
    bool darkMode_{false};
};
