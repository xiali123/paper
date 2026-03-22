#include "MainWindow.hpp"
#include "SearchWidget.hpp"
#include "ResultView.hpp"
#include "ProgressView.hpp"
#include "FilterPanel.hpp"
#include "ThemeManager.hpp"
#include "HeroWidget.hpp"
#include "FeatureCards.hpp"
#include "PaperCardView.hpp"
#include "ApiManager.hpp"
#include "PaperCache.hpp"
#include "ExportManager.hpp"
// #include "database/LocalDatabase.hpp"  // TODO: Re-enable after type system refactoring
#include <QTimer>
#include <QCloseEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QScrollArea>

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QApplication>
#include <QStyleFactory>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {

    // Initialize theme manager first
    themeManager_ = new ThemeManager(this);
    themeManager_->applyTheme();

    // Initialize API manager
    apiManager_ = new ApiManager(this);

    // Initialize paper cache for pagination optimization
    paperCache_ = new PaperCache(this);
    paperCache_->setMaxCachePages(20);  // Cache up to 20 pages per search

    // Initialize export manager
    exportManager_ = new ExportManager(this);
    connect(exportManager_, &ExportManager::exportCompleted,
            this, [this](const QString& fileName, int count) {
        QMessageBox::information(this, "导出成功",
            QString("成功导出 %1 篇论文到:\n%2").arg(count).arg(fileName));
        statusBar()->showMessage("导出成功: " + fileName, 5000);
    });
    connect(exportManager_, &ExportManager::exportFailed,
            this, [this](const QString& error) {
        QMessageBox::warning(this, "导出失败", error);
    });

    // TODO: Re-enable database after type system refactoring
    // localDb_ = new LocalDatabase(this);
    // if (!localDb_->open()) {
    //     QMessageBox::warning(this, "数据库错误",
    //                        "无法打开本地数据库，某些功能可能不可用");
    // }
    localDb_ = nullptr;

    setWindowTitle("📚 PaperCrawler - Academic Paper Search Tool");
    resize(1400, 900);  // Increased from 1200x800 for better display

    setupUI();
    createMenus();
    createToolBar();
    createThemeButton();
    connectSignals();
    loadSettings();

    // Check API health on startup
    apiManager_->checkHealth();

    statusBar()->showMessage("Ready - PaperCrawler Desktop v1.0", 3000);
}

MainWindow::~MainWindow() {
    saveSettings();

    // Clean up pointers
    if (themeManager_) {
        delete themeManager_;
        themeManager_ = nullptr;
    }
    if (apiManager_) {
        delete apiManager_;
        apiManager_ = nullptr;
    }
    if (paperCache_) {
        delete paperCache_;
        paperCache_ = nullptr;
    }
    if (localDb_) {
        delete localDb_;
        localDb_ = nullptr;
    }
}

void MainWindow::setupUI() {
    // Create central widget with scroll area
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setObjectName("mainScrollArea");

    auto* centralWidget = new QWidget();
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Hero Section
    heroWidget_ = new HeroWidget(this);
    mainLayout->addWidget(heroWidget_);

    // Feature Cards
    featureCards_ = new FeatureCards(this);
    mainLayout->addWidget(featureCards_);

    // Search Widget
    searchWidget_ = new SearchWidget(this);
    mainLayout->addWidget(searchWidget_);

    // Results Section (initially hidden)
    resultView_ = new PaperCardView(this);
    resultView_->setVisible(false);
    mainLayout->addWidget(resultView_);

    // Add stretch at bottom
    mainLayout->addStretch();

    scrollArea->setWidget(centralWidget);
    setCentralWidget(scrollArea);
}

void MainWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    // Draw gradient background
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Create gradient matching web frontend
    QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
    auto colors = themeManager_->colors();

    gradient.setColorAt(0, colors.backgroundStart);
    gradient.setColorAt(1, colors.backgroundEnd);

    painter.fillRect(rect(), gradient);
}

void MainWindow::createMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    // Export sub-menu
    QMenu* exportMenu = fileMenu->addMenu("&Export");

    QAction* exportCSVAction = exportMenu->addAction("Export to &CSV");
    exportCSVAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportCSVAction, &QAction::triggered, this, [this]() {
        onExport(ExportFormat::CSV);
    });

    QAction* exportBibTeXAction = exportMenu->addAction("Export to &BibTeX");
    exportBibTeXAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    connect(exportBibTeXAction, &QAction::triggered, this, [this]() {
        onExport(ExportFormat::BibTeX);
    });

    QAction* exportJSONAction = exportMenu->addAction("Export to &JSON");
    exportJSONAction->setShortcut(QKeySequence("Ctrl+Alt+E"));
    connect(exportJSONAction, &QAction::triggered, this, [this]() {
        onExport(ExportFormat::JSON);
    });

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    // Edit menu
    QMenu* editMenu = menuBar()->addMenu("&Edit");

    QAction* preferencesAction = editMenu->addAction("&Preferences");
    preferencesAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(preferencesAction, &QAction::triggered, this, &MainWindow::onPreferences);

    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");

    QAction* themeAction = viewMenu->addAction("&Toggle Theme");
    themeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(themeAction, &QAction::triggered, this, &MainWindow::onToggleTheme);

    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");

    QAction* statsAction = toolsMenu->addAction("&Statistics");
    statsAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(statsAction, &QAction::triggered, this, &MainWindow::onShowStatistics);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::createToolBar() {
    QToolBar* toolBar = addToolBar("Main Toolbar");
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    toolBar->setStyleSheet(
        "QToolBar {"
        "  background-color: rgba(255, 255, 255, 0.95);"
        "  border: none;"
        "  spacing: 8px;"
        "  padding: 8px;"
        "  border-radius: 10px;"
        "  margin: 4px;"
        "}"
        "QToolButton {"
        "  background-color: transparent;"
        "  border-radius: 8px;"
        "  padding: 8px 16px;"
        "}"
        "QToolButton:hover {"
        "  background-color: rgba(99, 102, 241, 0.1);"
        "}"
    );

    QAction* searchAction = toolBar->addAction("🔍 Search");
    connect(searchAction, &QAction::triggered, this, [this]() {
        searchWidget_->setFocus();
        statusBar()->showMessage("Enter keyword in search box above", 3000);
    });

    toolBar->addSeparator();

    QAction* exportAction = toolBar->addAction("📥 Export");
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExport);

    toolBar->addSeparator();

    QAction* settingsAction = toolBar->addAction("⚙ Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onPreferences);
}

void MainWindow::createThemeButton() {
    // Create floating theme toggle button
    themeButton_ = new QPushButton(this);
    themeButton_->setText("🌙");
    themeButton_->setToolTip("Toggle Dark/Light Theme");
    themeButton_->setObjectName("themeButton");
    themeButton_->setFixedSize(56, 56);
    themeButton_->setCursor(Qt::PointingHandCursor);

    // Position in top-right corner
    themeButton_->move(width() - 70, 80);

    // Modern button styling
    themeButton_->setStyleSheet(
        "QPushButton#themeButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "    stop:0 rgba(255, 255, 255, 0.95), "
        "    stop:1 rgba(243, 244, 246, 0.95));"
        "  border: 2px solid rgba(99, 102, 241, 0.3);"
        "  border-radius: 28px;"
        "  font-size: 24pt;"
        "  color: #6366f1;"
        "}"
        "QPushButton#themeButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "    stop:0 #667eea, stop:1 #764ba2);"
        "  color: white;"
        "  transform: scale(1.1);"
        "}"
        "QPushButton#themeButton:pressed {"
        "  transform: scale(0.95);"
        "}"
    );

    connect(themeButton_, &QPushButton::clicked,
            this, &MainWindow::onToggleTheme);
}

void MainWindow::connectSignals() {
    connect(searchWidget_, &SearchWidget::searchRequested,
            this, &MainWindow::onSearch);

    if (resultView_) {
        connect(resultView_, &PaperCardView::paperSelected,
                this, &MainWindow::onPaperSelected);
        connect(resultView_, &PaperCardView::pageChanged,
                this, &MainWindow::onPageChanged);
    }

    if (tableView_) {
        connect(tableView_, &ResultView::paperSelected,
                this, &MainWindow::onPaperSelected);
    }

    // Connect API signals
    connect(apiManager_, &ApiManager::searchSuccess,
            this, &MainWindow::onSearchSuccess);
    connect(apiManager_, &ApiManager::searchFailed,
            this, &MainWindow::onSearchFailed);
    connect(apiManager_, &ApiManager::healthCheckSuccess,
            this, &MainWindow::onHealthCheckSuccess);
    connect(apiManager_, &ApiManager::networkError,
            this, &MainWindow::onNetworkError);
}

void MainWindow::loadSettings() {
    QSettings settings("PaperCrawler", "Desktop");

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::saveSettings() {
    QSettings settings("PaperCrawler", "Desktop");

    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::onSearch(const QString& keyword) {
    if (keyword.isEmpty()) {
        QMessageBox::warning(this, "搜索",
                           "请输入搜索关键词。");
        return;
    }

    // Save search state for pagination
    currentKeyword_ = keyword;
    currentOffset_ = 0;
    currentLimit_ = 20;

    statusBar()->showMessage("正在搜索: " + keyword + "...");
    resultView_->setVisible(true);
    resultView_->clear();

    // Clear cache for this keyword if starting fresh search
    // (Optional: keep cache for faster access if same keyword searched again)
    // paperCache_->clear(keyword);

    // Check cache first
    QList<Paper> cachedPapers;
    int cachedTotal = 0;

    if (paperCache_->get(keyword, 0, 20, cachedPapers, cachedTotal)) {
        qDebug() << "Fresh search found in cache! Displaying cached results.";
        resultView_->setPapers(cachedPapers, cachedTotal, 1);  // Page 1
        statusBar()->showMessage(QString("搜索完成（来自缓存）！找到 %1 篇相关论文").arg(cachedTotal), 5000);
        return;
    }

    // TODO: Add local database search later
    // For now, directly search backend
    apiManager_->searchPapers(keyword, "", "", currentOffset_, currentLimit_);
}

void MainWindow::onPageChanged(int offset, int limit) {
    qDebug() << "=== MainWindow::onPageChanged ===";
    qDebug() << "Offset:" << offset << "Limit:" << limit;
    qDebug() << "Current keyword:" << currentKeyword_;

    if (currentKeyword_.isEmpty()) {
        qWarning() << "No current keyword, ignoring page change";
        return;
    }

    currentOffset_ = offset;
    currentLimit_ = limit;

    int pageNum = (offset / limit) + 1;

    // Try to get from cache first
    QList<Paper> cachedPapers;
    int cachedTotal = 0;

    if (paperCache_->get(currentKeyword_, offset, limit, cachedPapers, cachedTotal)) {
        qDebug() << "Cache HIT! Displaying cached papers for page" << pageNum;

        // Display cached results immediately with correct page number
        resultView_->setPapers(cachedPapers, cachedTotal, pageNum);

        QString message = QString("第 %1 页（来自缓存）- 共 %2 篇论文").arg(pageNum).arg(cachedTotal);
        statusBar()->showMessage(message, 3000);

        return;
    }

    // Cache miss - fetch from backend
    qDebug() << "Cache MISS - fetching from backend API";
    statusBar()->showMessage(QString("正在加载第 %1 页...").arg(pageNum));

    // Call API with new offset and limit
    apiManager_->searchPapers(currentKeyword_, "", "", currentOffset_, currentLimit_);
}

void MainWindow::onPaperSelected(int paperId) {
    // Call API to get paper details
    apiManager_->getPaperDetails(paperId);
    statusBar()->showMessage(QString("正在加载论文 %1 的详细信息...").arg(paperId));
}

void MainWindow::onSearchSuccess(const SearchResult& result) {
    // Save total count for pagination
    totalResults_ = result.total;

    qDebug() << "=== Backend Search Results ===";
    qDebug() << "Papers received:" << result.papers.size();
    qDebug() << "Total papers:" << result.total;
    qDebug() << "Offset:" << currentOffset_ << "Limit:" << currentLimit_;

    // Convert ApiPaper to Paper
    QList<Paper> papers;
    for (const auto& apiPaper : result.papers) {
        Paper paper;
        paper.id = apiPaper.id;
        paper.title = apiPaper.title;
        paper.journal = apiPaper.journalShort.isEmpty()
                       ? apiPaper.journalFull
                       : apiPaper.journalShort;
        paper.year = apiPaper.year;
        paper.level = apiPaper.level;
        paper.authors = apiPaper.authors;
        paper.doiUrl = apiPaper.doiUrl;
        papers.append(paper);
    }

    // Cache the results for this page
    qDebug() << "Caching results for" << currentKeyword_ << "offset=" << currentOffset_;
    paperCache_->insert(currentKeyword_, currentOffset_, currentLimit_, papers, totalResults_);

    // Display results with correct page number
    int pageNum = (currentOffset_ / currentLimit_) + 1;
    resultView_->setPapers(papers, totalResults_, pageNum);

    // Update status bar
    QString message = QString("第 %1 页 - 搜索完成！找到 %2 篇相关论文")
                         .arg(pageNum).arg(result.total);
    if (result.durationMs > 0) {
        message += QString(" (耗时 %1 ms)").arg(result.durationMs, 0, 'f', 2);
    }
    statusBar()->showMessage(message, 5000);

    // Log cache statistics
    qDebug() << "Cache statistics for" << currentKeyword_ << ":"
             << "Cached pages:" << paperCache_->getCacheCount(currentKeyword_)
             << "Total cache size:" << paperCache_->getCacheSize();
}

void MainWindow::onSearchFailed(const QString& error) {
    resultView_->clear();
    QMessageBox::warning(this, "搜索失败",
        QString("搜索论文时出错：\n%1\n\n"
                "请检查：\n"
                "1. 后端服务是否运行 (http://localhost:8080)\n"
                "2. 网络连接是否正常").arg(error));
    statusBar()->showMessage("搜索失败", 3000);
}

void MainWindow::onHealthCheckSuccess(bool healthy, const QString& message) {
    if (heroWidget_) {
        heroWidget_->setHealthStatus(healthy
            ? HeroWidget::HealthStatus::Healthy
            : HeroWidget::HealthStatus::Unhealthy);
    }

    if (healthy) {
        qDebug() << "API Health check: OK -" << message;
    } else {
        qDebug() << "API Health check: FAILED -" << message;
    }
}

void MainWindow::onNetworkError(const QString& error) {
    qWarning() << "Network error:" << error;
    statusBar()->showMessage("网络错误: " + error.left(50) + "...", 5000);
}

void MainWindow::onExport(ExportFormat format) {
    if (!resultView_ || resultView_->paperCount() == 0) {
        QMessageBox::warning(this, "导出",
            "没有可导出的论文。\n请先进行搜索。");
        return;
    }

    // Show save dialog
    QString fileName = exportManager_->showSaveDialog(this, format);

    if (fileName.isEmpty()) {
        return;  // User cancelled
    }

    statusBar()->showMessage("正在导出到: " + fileName + "...");

    // Get papers from result view
    // Note: We need to access the papers from PaperCardView
    // For now, we'll need to add a getter to PaperCardView
    // This is a placeholder - you'll need to implement getPapers() in PaperCardView

    // TODO: Get actual papers from result view
    // QList<Paper> papers = resultView_->getPapers();

    // For now, show a message
    QMessageBox::information(this, "导出",
        "导出功能已创建！\n\n"
        "需要在 PaperCardView 中添加 getPapers() 方法\n"
        "来返回当前显示的论文列表。\n\n"
        "支持格式：CSV, BibTeX, JSON");
}

void MainWindow::onPreferences() {
    QMessageBox::information(this, "Preferences",
        "Preferences dialog will be implemented.\n\n"
        "Settings will include:\n"
        "- API endpoint configuration\n"
        "- Theme selection\n"
        "- Export formats\n"
        "- Search preferences");
}

void MainWindow::onToggleTheme() {
    if (themeManager_) {
        themeManager_->toggleTheme();

        // Update theme button icon
        QString icon = themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark
                      ? "☀️" : "🌙";
        themeButton_->setText(icon);

        // Update status bar
        QString themeName = themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark
                           ? "Dark" : "Light";
        statusBar()->showMessage(themeName + " theme enabled - Enjoy the modern UI!", 3000);

        // Trigger repaint for gradient update
        update();
    }
}

void MainWindow::onShowStatistics() {
    QMessageBox::information(this, "Statistics",
        "Statistics view will be implemented.\n\n"
        "Will show:\n"
        "- Total papers in database\n"
        "- Papers by year\n"
        "- Top journals\n"
        "- Publication trends");
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About PaperCrawler",
        "<h2>📚 PaperCrawler Desktop</h2>"
        "<p>Version 1.0.0</p>"
        "<p>A modern academic paper search and management tool.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Search papers from DBLP</li>"
        "<li>Fetch journal information</li>"
        "<li>Export to CSV/JSON/BibTeX</li>"
        "<li>Beautiful Qt6 GUI</li>"
        "</ul>"
        "<p><b>Web Version:</b> Run START-WEB.bat and visit http://localhost:5173</p>"
        "<p>&copy; 2024 PaperCrawler Project</p>");
}

void MainWindow::onPaperAdded(int paperId) {
    qDebug() << "Paper added to database:" << paperId;
}

void MainWindow::onDatabaseError(const QString& error) {
    qWarning() << "Database error:" << error;
    statusBar()->showMessage("数据库错误: " + error.left(50), 5000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Exit", "Are you sure you want to exit?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}
