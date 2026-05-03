#include "MainWindow.hpp"
#include "SearchWidget.hpp"
#include "LatexEditorWidget.hpp"
#include "LatexPreviewWidget.hpp"
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
#include "AuthManager.hpp"
#include "LoginWindow.hpp"
#include "PaperDetailDialog.hpp"
#include "SettingsDialog.hpp"
#include "FavoriteManager.hpp"
#include "SearchHistory.hpp"
#include "database/LocalDatabase.hpp"
#include "ToastWidget.hpp"
#include "PaperNotesDialog.hpp"
#include "ReadingListManager.hpp"
#include "AdvancedSearchDialog.hpp"
#include "OfflineCacheManager.hpp"
#include "PaperCompareDialog.hpp"
#include "RecentHistoryWidget.hpp"
#include "BatchOperationsBar.hpp"
#include "SystemTrayManager.hpp"
#include "WelcomeWidget.hpp"
#include "CitationGraphWidget.hpp"
#include "PaperTimelineWidget.hpp"
#include "ShortcutConfigDialog.hpp"
#include <QTimer>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QScrollArea>

#include <QMenuBar>
#include <QShortcut>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTableWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QFile>
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
#include <functional>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {

    // Initialize theme manager first
    themeManager_ = new ThemeManager(this);
    themeManager_->applyTheme();

    // Initialize API manager
    apiManager_ = new ApiManager(this);

    // Initialize paper cache for pagination optimization
    paperCache_ = new PaperCache(this);
    paperCache_->setMaxCachePages(20);

    // Initialize favorite manager and search history
    auto* favoriteManager = new FavoriteManager(this);
    auto* searchHistory = new SearchHistory(this);

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

    localDb_ = new LocalDatabase(this);
    if (!localDb_->open()) {
        qDebug() << "Local database unavailable";
        localDb_->close();
    }

    setWindowTitle("📚 PaperCrawler - Academic Paper Search Tool");
    resize(1400, 900);  // Increased from 1200x800 for better display

    setupUI();
    createMenus();
    createToolBar();
    createThemeButton();
    connectSignals();
    loadSettings();

    initializeAuthentication();

    // Keyboard shortcuts
    auto* searchShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    connect(searchShortcut, &QShortcut::activated, this, [this]() {
        searchWidget_->setFocus();
    });
    auto* exportShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_E), this);
    connect(exportShortcut, &QShortcut::activated, this, &MainWindow::onExport);
    auto* themeShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this);
    connect(themeShortcut, &QShortcut::activated, this, &MainWindow::onToggleTheme);
    auto* statsShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_U), this);
    connect(statsShortcut, &QShortcut::activated, this, &MainWindow::onShowStatistics);
    auto* settingsShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this);
    connect(settingsShortcut, &QShortcut::activated, this, &MainWindow::onPreferences);

    // Tab switch shortcuts Ctrl+1..8
    for (int i = 1; i <= 8; ++i) {
        auto* sc = new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i - 1)), this);
        connect(sc, &QShortcut::activated, this, [this, i]() {
            if (tabWidget_ && i < tabWidget_->count()) tabWidget_->setCurrentIndex(i);
        });
    }

    // Check API health on startup
    healthTimer_.start();
    apiManager_->checkHealth();

    // Periodic health check every 30 seconds
    auto* healthCheckTimer = new QTimer(this);
    connect(healthCheckTimer, &QTimer::timeout, this, [this]() {
        healthTimer_.restart();
        apiManager_->checkHealth();
    });
    healthCheckTimer->start(30000);

    statusBar()->showMessage("Ready - Ctrl+F: Search | Ctrl+E: Export | Ctrl+T: Theme | Ctrl+,: Settings", 5000);

    // System tray icon
    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setToolTip("PaperCrawler Desktop");
    trayIcon_->setIcon(windowIcon());

    auto* trayMenu = new QMenu(this);
    trayMenu->addAction("Show", this, &MainWindow::showNormal);
    trayMenu->addAction("Search", this, [this]() {
        showNormal();
        searchWidget_->setFocus();
    });
    trayMenu->addSeparator();
    trayMenu->addAction("Quit", QApplication::quit);
    trayIcon_->setContextMenu(trayMenu);

    connect(trayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            activateWindow();
        }
    });

    trayIcon_->show();
}

MainWindow::~MainWindow() {
    saveSettings();
    // Qt parent-child mechanism auto-deletes children
}

void MainWindow::setupUI() {
    // Tab widget as central widget
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabPosition(QTabWidget::South);
    tabWidget_->setDocumentMode(true);
    tabWidget_->setStyleSheet(
        "QTabWidget::pane { border: none; }"
        "QTabBar::tab { padding: 10px 28px; font-weight: bold; font-size: 12px; "
        "  border: none; border-top: 2px solid transparent; }"
        "QTabBar::tab:selected { color: #4f46e5; border-top: 2px solid #4f46e5; }"
        "QTabBar::tab:hover { color: #6366f1; background: #f8fafc; }"
    );
    setCentralWidget(tabWidget_);

    // === Tab 1: Search ===
    auto* searchScroll = new QScrollArea();
    searchScroll->setWidgetResizable(true);
    searchScroll->setFrameShape(QFrame::NoFrame);

    auto* searchPage = new QWidget();
    auto* searchLayout = new QVBoxLayout(searchPage);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(0);

    heroWidget_ = new HeroWidget(this);
    searchLayout->addWidget(heroWidget_);

    featureCards_ = new FeatureCards(this);
    searchLayout->addWidget(featureCards_);

    searchWidget_ = new SearchWidget(this);
    searchWidget_->setSearchHistory(searchHistory);
    searchLayout->addWidget(searchWidget_);

    // Search history quick access row
    {
        auto* historySection = new QWidget();
        historySection->setObjectName("searchHistorySection");
        auto* historyLayout = new QHBoxLayout(historySection);
        historyLayout->setContentsMargins(24, 4, 24, 4);
        historyLayout->setSpacing(6);

        auto* historyLabel = new QLabel("Recent:");
        historyLabel->setStyleSheet("color: #94a3b8; font-size: 12px; font-weight: bold;");
        historyLayout->addWidget(historyLabel);

        historyLayout->addStretch();

        auto* historyGrid = historyLayout;

        // Populate on load and after search
        connect(searchHistory, &SearchHistory::historyAdded, this, [this, historyGrid, searchHistory]() {
            // Clear old buttons
            while (historyGrid->count() > 2) {
                auto* item = historyGrid->takeAt(1);
                delete item->widget();
                delete item;
            }

            auto keywords = searchHistory->getRecentKeywords(5);
            for (const auto& kw : keywords) {
                auto* btn = new QPushButton(kw);
                btn->setStyleSheet(
                    "QPushButton { background: palette(base); border: 1px solid palette(mid); "
                    "border-radius: 12px; padding: 3px 12px; font-size: 11px; color: palette(text); }"
                    "QPushButton:hover { background: #e0e7ff; }"
                );
                btn->setCursor(Qt::PointingHandCursor);
                btn->setMaximumWidth(120);
                connect(btn, &QPushButton::clicked, this, [this, kw]() {
                    onSearch(kw);
                });
                historyGrid->insertWidget(historyGrid->count() - 1, btn);
            }
        });

        searchLayout->addWidget(historySection);
    }

    auto* resultsLayout = new QHBoxLayout();
    resultsLayout->setSpacing(12);

    resultView_ = new PaperCardView(this);
    resultView_->setFavoriteManager(favoriteManager);
    resultView_->setVisible(false);
    resultsLayout->addWidget(resultView_, 1);

    filterPanel_ = new FilterPanel(this);
    filterPanel_->setMaximumWidth(200);
    filterPanel_->setVisible(false);
    resultsLayout->addWidget(filterPanel_);

    searchLayout->addLayout(resultsLayout);

    // Recent papers section
    {
        auto* recentSection = new QWidget();
        recentSection->setObjectName("recentPapersSection");
        auto* recentLayout = new QVBoxLayout(recentSection);
        recentLayout->setContentsMargins(24, 16, 24, 16);
        recentLayout->setSpacing(8);

        auto* recentHeader = new QLabel("Recent Papers");
        recentHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        recentLayout->addWidget(recentHeader);

        auto* recentGrid = new QWidget();
        recentGrid->setObjectName("recentPapersGrid");
        auto* gridLayout = new QHBoxLayout(recentGrid);
        gridLayout->setObjectName("recentGridLayout");
        gridLayout->setSpacing(12);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        recentLayout->addWidget(recentGrid);

        searchLayout->addWidget(recentSection);
    }

    // Add paper button
    {
        auto* addPaperBar = new QHBoxLayout();
        addPaperBar->addStretch();
        auto* addPaperBtn = new QPushButton("+ Add Paper");
        addPaperBtn->setObjectName("addPaperBtn");
        addPaperBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; border-radius: 8px; "
            "padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(addPaperBtn, &QPushButton::clicked, this, [this]() {
            QDialog dlg(this);
            dlg.setWindowTitle("Add New Paper");
            dlg.setMinimumWidth(500);
            auto* form = new QVBoxLayout(&dlg);

            auto* titleEdit = new QLineEdit();
            titleEdit->setPlaceholderText("Title *");
            titleEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(titleEdit);

            auto* authorsEdit = new QLineEdit();
            authorsEdit->setPlaceholderText("Authors (comma-separated)");
            authorsEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(authorsEdit);

            auto* journalEdit = new QLineEdit();
            journalEdit->setPlaceholderText("Journal");
            journalEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(journalEdit);

            auto* yearEdit = new QLineEdit();
            yearEdit->setPlaceholderText("Year");
            yearEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(yearEdit);

            auto* levelCombo = new QComboBox();
            levelCombo->addItems({"", "A", "B", "C"});
            levelCombo->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(levelCombo);

            auto* doiEdit = new QLineEdit();
            doiEdit->setPlaceholderText("DOI URL");
            doiEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(doiEdit);

            auto* abstractEdit = new QTextEdit();
            abstractEdit->setPlaceholderText("Abstract");
            abstractEdit->setMaximumHeight(100);
            abstractEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(abstractEdit);

            auto* btnRow = new QHBoxLayout();
            auto* submitBtn = new QPushButton("Submit");
            submitBtn->setStyleSheet(
                "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
                "padding: 8px 24px; font-weight: bold; }"
            );
            auto* cancelBtn = new QPushButton("Cancel");
            cancelBtn->setStyleSheet(
                "QPushButton { background: palette(button); color: palette(button-text); "
                "border: 1px solid palette(mid); border-radius: 6px; padding: 8px 24px; }"
            );
            btnRow->addStretch();
            btnRow->addWidget(cancelBtn);
            btnRow->addWidget(submitBtn);
            form->addLayout(btnRow);

            connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
            connect(submitBtn, &QPushButton::clicked, this, [this, &dlg, titleEdit, authorsEdit,
                                                             journalEdit, yearEdit, levelCombo,
                                                             doiEdit, abstractEdit]() {
                if (titleEdit->text().trimmed().isEmpty()) {
                    titleEdit->setStyleSheet("padding: 8px; border: 2px solid #dc2626; border-radius: 6px;");
                    return;
                }
                QJsonObject data;
                data["title"] = titleEdit->text().trimmed();
                data["authors"] = authorsEdit->text().trimmed();
                data["journal"] = journalEdit->text().trimmed();
                data["year"] = yearEdit->text().trimmed();
                data["level"] = levelCombo->currentText();
                data["doi_url"] = doiEdit->text().trimmed();
                data["abstract"] = abstractEdit->toPlainText().trimmed();
                apiManager_->createPaper(data);
                statusBar()->showMessage("Paper created successfully", 5000);
                dlg.accept();
            });

            dlg.exec();
        });
        addPaperBar->addWidget(addPaperBtn);
        searchLayout->addLayout(addPaperBar);
    }

    // Advanced search section (saved searches + trending)
    {
        auto* advSection = new QWidget();
        advSection->setObjectName("advancedSearchSection");
        auto* advLayout = new QVBoxLayout(advSection);
        advLayout->setContentsMargins(24, 8, 24, 8);
        advLayout->setSpacing(8);

        // Saved searches row
        auto* savedRow = new QHBoxLayout();
        auto* savedLabel = new QLabel("Saved Searches:");
        savedLabel->setStyleSheet("font-weight: bold; color: palette(mid); font-size: 12px;");
        savedRow->addWidget(savedLabel);

        auto* savedList = new QWidget();
        savedList->setObjectName("savedSearchesList");
        auto* savedListLayout = new QHBoxLayout(savedList);
        savedListLayout->setContentsMargins(0, 0, 0, 0);
        savedListLayout->setSpacing(6);
        savedListLayout->addStretch();
        savedRow->addWidget(savedList, 1);

        auto* saveSearchBtn = new QPushButton("Save Current");
        saveSearchBtn->setObjectName("saveSearchBtn");
        saveSearchBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(saveSearchBtn, &QPushButton::clicked, this, [this]() {
            if (currentKeyword_.isEmpty()) {
                statusBar()->showMessage("Search first before saving", 3000);
                return;
            }
            QJsonObject data;
            data["query"] = currentKeyword_;
            data["name"] = currentKeyword_.left(30);
            apiManager_->saveSearch(data);
            statusBar()->showMessage("Search saved: " + currentKeyword_, 3000);
        });
        savedRow->addWidget(saveSearchBtn);

        auto* loadSavedBtn = new QPushButton("Load");
        loadSavedBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 4px 12px; font-size: 11px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        connect(loadSavedBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getSavedSearches();
        });
        savedRow->addWidget(loadSavedBtn);

        auto* allPapersBtn = new QPushButton("All Papers");
        allPapersBtn->setStyleSheet(loadSavedBtn->styleSheet());
        connect(allPapersBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getAllPapers(1, 50);
            statusBar()->showMessage("Loading all papers...", 3000);
        });
        savedRow->addWidget(allPapersBtn);

        auto* searchHistoryBtn = new QPushButton("History");
        searchHistoryBtn->setStyleSheet(loadSavedBtn->styleSheet());
        connect(searchHistoryBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getSearchHistory();
            statusBar()->showMessage("Loading search history...", 3000);
        });
        savedRow->addWidget(searchHistoryBtn);

        advLayout->addLayout(savedRow);

        // Trending searches row
        auto* trendingRow = new QHBoxLayout();
        auto* trendingLabel = new QLabel("Trending:");
        trendingLabel->setStyleSheet("font-weight: bold; color: palette(mid); font-size: 12px;");
        trendingRow->addWidget(trendingLabel);

        auto* trendingList = new QWidget();
        trendingList->setObjectName("trendingSearchesList");
        auto* trendingListLayout = new QHBoxLayout(trendingList);
        trendingListLayout->setContentsMargins(0, 0, 0, 0);
        trendingListLayout->setSpacing(6);
        trendingListLayout->addStretch();
        trendingRow->addWidget(trendingList, 1);

        auto* refreshTrendingBtn = new QPushButton("Refresh");
        refreshTrendingBtn->setStyleSheet(loadSavedBtn->styleSheet());
        connect(refreshTrendingBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getTrendingSearches();
        });
        trendingRow->addWidget(refreshTrendingBtn);

        advLayout->addLayout(trendingRow);
        searchLayout->addWidget(advSection);
    }

    searchLayout->addStretch();

    searchScroll->setWidget(searchPage);
    tabWidget_->addTab(searchScroll, "\xf0\x9f\x94\x8d Search");

    // === Tab 2: Favorites ===
    {
        auto* page = new QWidget();
        page->setObjectName("favoritesPage");
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);

        auto* headerRow = new QHBoxLayout();
        auto* header = new QLabel("My Favorites");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        headerRow->addWidget(header);
        headerRow->addStretch();

        auto* countLabel = new QLabel("0 papers saved");
        countLabel->setObjectName("favCountLabel");
        countLabel->setStyleSheet("color: palette(mid); font-size: 13px;");
        headerRow->addWidget(countLabel);

        // Batch export button
        auto* exportFavBtn = new QPushButton("Export All");
        exportFavBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(exportFavBtn, &QPushButton::clicked, this, [this, favoriteManager]() {
            auto favs = favoriteManager->getFavorites();
            if (favs.isEmpty()) {
                QMessageBox::information(this, "Export", "No favorites to export.");
                return;
            }
            QList<Paper> papers;
            for (const auto& f : favs) {
                Paper p;
                p.id = f.paperId;
                p.title = f.title;
                p.journalFull = f.journal;
                p.year = f.year;
                papers.append(p);
            }
            QString fileName = exportManager_->showSaveDialog(this, ExportFormat::CSV);
            if (!fileName.isEmpty()) {
                exportManager_->exportToCSV(fileName, papers);
            }
        });
        headerRow->addWidget(exportFavBtn);

        // Clear all button
        auto* clearFavBtn = new QPushButton("Clear All");
        clearFavBtn->setStyleSheet(
            "QPushButton { background: none; border: 1px solid #ef4444; color: #ef4444; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #fef2f2; }"
        );
        connect(clearFavBtn, &QPushButton::clicked, this, [this, favoriteManager]() {
            auto reply = QMessageBox::question(this, "Clear Favorites",
                "Remove all favorites?",
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                favoriteManager->clear();
                refreshFavoritesTab();
            }
        });
        headerRow->addWidget(clearFavBtn);

        layout->addLayout(headerRow);

        auto* favScroll = new QScrollArea();
        favScroll->setWidgetResizable(true);
        favScroll->setFrameShape(QFrame::NoFrame);
        auto* favContent = new QWidget();
        auto* favListLayout = new QVBoxLayout(favContent);
        favListLayout->setObjectName("favListLayout");
        favListLayout->setAlignment(Qt::AlignTop);
        favListLayout->setSpacing(8);
        favScroll->setWidget(favContent);
        layout->addWidget(favScroll, 1);

        // Refresh favorites when toggled
        auto* favMgr = favoriteManager;
        if (favMgr) {
            connect(favMgr, &FavoriteManager::favoriteAdded, this, [this]() {
                if (tabWidget_->currentIndex() == 1) refreshFavoritesTab();
            });
            connect(favMgr, &FavoriteManager::favoriteRemoved, this, [this]() {
                if (tabWidget_->currentIndex() == 1) refreshFavoritesTab();
            });
        }

        tabWidget_->addTab(page, "\xe2\xad\x90 Favorites");
    }

    // === Tab 3: Crawler ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);

        auto* headerRow = new QHBoxLayout();
        auto* header = new QLabel("Crawler Dashboard");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        headerRow->addWidget(header);
        headerRow->addStretch();

        auto* refreshBtn = new QPushButton("Refresh");
        refreshBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(refreshBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerTasks();
            apiManager_->getCrawlerDashboard();
        });
        headerRow->addWidget(refreshBtn);

        // New Task button
        auto* newTaskBtn = new QPushButton("+ New Task");
        newTaskBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(newTaskBtn, &QPushButton::clicked, this, [this]() {
            auto* dlg = new QDialog(this);
            dlg->setWindowTitle("New Crawler Task");
            dlg->setMinimumWidth(400);
            auto* form = new QVBoxLayout(dlg);

            auto* urlLabel = new QLabel("URL to crawl:");
            form->addWidget(urlLabel);
            auto* urlInput = new QLineEdit();
            urlInput->setPlaceholderText("https://dblp.org/...");
            form->addWidget(urlInput);

            auto* sourceLabel = new QLabel("Source:");
            form->addWidget(sourceLabel);
            auto* sourceCombo = new QComboBox();
            sourceCombo->addItems({"dblp", "semantic_scholar", "crossref", "arxiv"});
            form->addWidget(sourceCombo);

            auto* btnRow = new QHBoxLayout();
            auto* cancelBtn = new QPushButton("Cancel");
            connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
            btnRow->addWidget(cancelBtn);
            auto* createBtn = new QPushButton("Create");
            createBtn->setStyleSheet(
                "QPushButton { background: #4f46e5; color: white; border: none; "
                "border-radius: 6px; padding: 6px 20px; font-weight: bold; }"
            );
            connect(createBtn, &QPushButton::clicked, this, [this, dlg, urlInput, sourceCombo]() {
                QString url = urlInput->text().trimmed();
                if (url.isEmpty()) return;
                QJsonObject data;
                data["url"] = url;
                data["source"] = sourceCombo->currentText();
                apiManager_->post(apiManager_->createRequest("/api/crawler/tasks"),
                    QJsonDocument(data).toJson());
                dlg->accept();
                statusBar()->showMessage("Crawler task created for: " + url, 5000);
                // Refresh after short delay
                QTimer::singleShot(1000, this, [this]() {
                    apiManager_->getCrawlerTasks();
                    apiManager_->getCrawlerDashboard();
                });
            });
            btnRow->addWidget(createBtn);
            form->addLayout(btnRow);

            dlg->exec();
            dlg->deleteLater();
        });
        headerRow->addWidget(newTaskBtn);

        auto* crawlerStatsBtn = new QPushButton("Statistics");
        crawlerStatsBtn->setStyleSheet(
            "QPushButton { background: #0891b2; color: white; border: none; "
            "border-radius: 6px; padding: 6px 14px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #0e7490; }"
        );
        connect(crawlerStatsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerStatistics();
            statusBar()->showMessage("Loading crawler statistics...", 3000);
        });
        headerRow->addWidget(crawlerStatsBtn);

        auto* workersBtn = new QPushButton("Workers");
        workersBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; "
            "border-radius: 6px; padding: 6px 14px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(workersBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerWorkers();
            statusBar()->showMessage("Loading crawler workers...", 3000);
        });
        headerRow->addWidget(workersBtn);

        layout->addLayout(headerRow);

        // Stats row
        auto* statsRow = new QHBoxLayout();
        statsRow->setSpacing(16);
        auto makeCard = [](const QString& title, const QString& value, const QString& color) {
            auto* card = new QWidget();
            card->setStyleSheet(
                QString("QWidget { background: %1; border-radius: 8px; padding: 16px; }").arg(color)
            );
            auto* l = new QVBoxLayout(card);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(4);
            auto* v = new QLabel(value);
            v->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
            v->setObjectName(title);
            l->addWidget(v);
            auto* t = new QLabel(title);
            t->setStyleSheet("font-size: 11px; color: rgba(255,255,255,0.8); font-weight: 500;");
            l->addWidget(t);
            return card;
        };
        statsRow->addWidget(makeCard("Total Tasks", "0", "#4f46e5"));
        statsRow->addWidget(makeCard("Running", "0", "#059669"));
        statsRow->addWidget(makeCard("Completed", "0", "#0891b2"));
        statsRow->addWidget(makeCard("Failed", "0", "#dc2626"));
        layout->addLayout(statsRow);

        auto* taskTable = new QTableWidget(0, 6);
        taskTable->setObjectName("crawlerTaskTable");
        taskTable->setHorizontalHeaderLabels({"ID", "Source", "URL", "Status", "Created", "Actions"});
        taskTable->horizontalHeader()->setStretchLastSection(true);
        taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        taskTable->setAlternatingRowColors(true);
        layout->addWidget(taskTable, 1);

        // Templates section
        auto* tmplHeader = new QLabel("Templates");
        tmplHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: palette(text); margin-top: 8px;");
        layout->addWidget(tmplHeader);

        auto* tmplRow = new QHBoxLayout();
        auto* tmplTable = new QTableWidget(0, 4);
        tmplTable->setObjectName("crawlerTemplatesTable");
        tmplTable->setHorizontalHeaderLabels({"Name", "Source", "Pattern", "Actions"});
        tmplTable->horizontalHeader()->setStretchLastSection(true);
        tmplTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tmplTable->setAlternatingRowColors(true);
        tmplTable->setMaximumHeight(150);
        tmplTable->setStyleSheet(
            "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
            "QHeaderView::section { background: palette(window); color: palette(text); "
            "font-weight: bold; padding: 4px; border: none; }"
            "QTableWidget::item { padding: 2px; color: palette(text); }"
        );

        auto* newTmplBtn = new QPushButton("+ Template");
        newTmplBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(newTmplBtn, &QPushButton::clicked, this, [this]() {
            QDialog dlg(this);
            dlg.setWindowTitle("New Crawler Template");
            dlg.setMinimumWidth(400);
            auto* form = new QVBoxLayout(&dlg);
            auto* nameEdit = new QLineEdit();
            nameEdit->setPlaceholderText("Template name");
            nameEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(nameEdit);
            auto* sourceCombo = new QComboBox();
            sourceCombo->addItems({"ieee", "acm", "springer", "arxiv", "elsevier", "custom"});
            sourceCombo->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(sourceCombo);
            auto* urlPatternEdit = new QLineEdit();
            urlPatternEdit->setPlaceholderText("URL pattern");
            urlPatternEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(urlPatternEdit);
            auto* btnRow = new QHBoxLayout();
            auto* okBtn = new QPushButton("Create");
            okBtn->setStyleSheet("QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; padding: 6px 20px; font-weight: bold; }");
            auto* cancelBtn = new QPushButton("Cancel");
            cancelBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 6px 20px; }");
            connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
            connect(okBtn, &QPushButton::clicked, this, [this, &dlg, nameEdit, sourceCombo, urlPatternEdit]() {
                QJsonObject data;
                data["name"] = nameEdit->text().trimmed();
                data["source"] = sourceCombo->currentText();
                data["url_pattern"] = urlPatternEdit->text().trimmed();
                apiManager_->createCrawlerTemplate(data);
                dlg.accept();
                statusBar()->showMessage("Template created", 3000);
                QTimer::singleShot(500, this, [this]() { apiManager_->getCrawlerTemplates(); });
            });
            btnRow->addStretch();
            btnRow->addWidget(cancelBtn);
            btnRow->addWidget(okBtn);
            form->addLayout(btnRow);
            dlg.exec();
        });

        auto* refreshTmplBtn = new QPushButton("Refresh");
        refreshTmplBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 6px 14px; font-size: 11px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        connect(refreshTmplBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerTemplates();
        });

        tmplRow->addWidget(tmplTable, 1);
        tmplRow->addWidget(newTmplBtn);
        tmplRow->addWidget(refreshTmplBtn);
        layout->addLayout(tmplRow);

        // Schedules section
        auto* schedHeader = new QLabel("Schedules");
        schedHeader->setStyleSheet("font-size: 14px; font-weight: bold; color: palette(text); margin-top: 4px;");
        layout->addWidget(schedHeader);

        auto* schedRow = new QHBoxLayout();
        auto* schedTable = new QTableWidget(0, 4);
        schedTable->setObjectName("crawlerSchedulesTable");
        schedTable->setHorizontalHeaderLabels({"ID", "Cron", "Template", "Actions"});
        schedTable->horizontalHeader()->setStretchLastSection(true);
        schedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        schedTable->setAlternatingRowColors(true);
        schedTable->setMaximumHeight(120);
        schedTable->setStyleSheet(tmplTable->styleSheet());

        auto* newSchedBtn = new QPushButton("+ Schedule");
        newSchedBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(newSchedBtn, &QPushButton::clicked, this, [this]() {
            QDialog dlg(this);
            dlg.setWindowTitle("New Crawler Schedule");
            auto* form = new QVBoxLayout(&dlg);
            auto* cronEdit = new QLineEdit();
            cronEdit->setPlaceholderText("Cron expression (e.g. 0 */6 * * *)");
            cronEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(cronEdit);
            auto* tmplIdEdit = new QLineEdit();
            tmplIdEdit->setPlaceholderText("Template ID");
            tmplIdEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
            form->addWidget(tmplIdEdit);
            auto* btnRow = new QHBoxLayout();
            auto* okBtn = new QPushButton("Create");
            okBtn->setStyleSheet("QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; padding: 6px 20px; font-weight: bold; }");
            auto* cancelBtn = new QPushButton("Cancel");
            cancelBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 6px 20px; }");
            connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
            connect(okBtn, &QPushButton::clicked, this, [this, &dlg, cronEdit, tmplIdEdit]() {
                QJsonObject data;
                data["cron"] = cronEdit->text().trimmed();
                data["template_id"] = tmplIdEdit->text().toInt();
                apiManager_->createCrawlerSchedule(data);
                dlg.accept();
                statusBar()->showMessage("Schedule created", 3000);
                QTimer::singleShot(500, this, [this]() { apiManager_->getCrawlerSchedules(); });
            });
            btnRow->addStretch();
            btnRow->addWidget(cancelBtn);
            btnRow->addWidget(okBtn);
            form->addLayout(btnRow);
            dlg.exec();
        });

        auto* refreshSchedBtn = new QPushButton("Refresh");
        refreshSchedBtn->setStyleSheet(refreshTmplBtn->styleSheet());
        connect(refreshSchedBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getCrawlerSchedules();
        });

        schedRow->addWidget(schedTable, 1);
        schedRow->addWidget(newSchedBtn);
        schedRow->addWidget(refreshSchedBtn);
        layout->addLayout(schedRow);

        // Connect templates & schedules responses
        connect(apiManager_, &ApiManager::crawlerTemplatesSuccess, this,
            [tmplTable](const QJsonArray& templates) {
                tmplTable->setRowCount(templates.size());
                for (int i = 0; i < templates.size(); ++i) {
                    auto t = templates[i].toObject();
                    tmplTable->setItem(i, 0, new QTableWidgetItem(t["name"].toString()));
                    tmplTable->setItem(i, 1, new QTableWidgetItem(t["source"].toString()));
                    tmplTable->setItem(i, 2, new QTableWidgetItem(t["url_pattern"].toString().left(40)));

                    auto* actions = new QWidget();
                    auto* al = new QHBoxLayout(actions);
                    al->setContentsMargins(4, 2, 4, 2);
                    al->setSpacing(4);
                    int tid = t["id"].toInt();

                    auto* testBtn = new QPushButton("Test");
                    testBtn->setStyleSheet("QPushButton { background: #0891b2; color: white; border: none; border-radius: 3px; padding: 2px 8px; font-size: 10px; }");
                    // testBtn would need ApiManager reference - skip for inline, use signal
                    al->addWidget(testBtn);
                    al->addStretch();
                    tmplTable->setCellWidget(i, 3, actions);
                }
            });

        connect(apiManager_, &ApiManager::crawlerTasksSuccess, this,
            [this, taskTable](const QJsonArray& tasks) {
                taskTable->setRowCount(tasks.size());
                for (int i = 0; i < tasks.size(); ++i) {
                    auto t = tasks[i].toObject();
                    taskTable->setItem(i, 0, new QTableWidgetItem(QString::number(t["id"].toInt())));
                    taskTable->setItem(i, 1, new QTableWidgetItem(t["source"].toString()));
                    taskTable->setItem(i, 2, new QTableWidgetItem(t["url"].toString().left(60)));

                    // Color-coded status
                    auto* statusItem = new QTableWidgetItem(t["status"].toString());
                    QString statusColor = t["status"].toString() == "completed" ? "#059669" :
                                          t["status"].toString() == "running" ? "#4f46e5" :
                                          t["status"].toString() == "failed" ? "#dc2626" : "#6b7280";
                    statusItem->setForeground(QColor(statusColor));
                    taskTable->setItem(i, 3, statusItem);

                    taskTable->setItem(i, 4, new QTableWidgetItem(t["created_at"].toString()));

                    // Action button
                    auto* actionWidget = new QWidget();
                    auto* actionLayout = new QHBoxLayout(actionWidget);
                    actionLayout->setContentsMargins(4, 2, 4, 2);
                    actionLayout->setSpacing(4);

                    int taskId = t["id"].toInt();
                    QString status = t["status"].toString();

                    if (status == "pending" || status == "paused") {
                        auto* startBtn = new QPushButton("Start");
                        startBtn->setStyleSheet(
                            "QPushButton { background: #059669; color: white; border: none; "
                            "border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
                        );
                        connect(startBtn, &QPushButton::clicked, this, [this, taskId]() {
                            apiManager_->put(apiManager_->createRequest(
                                QString("/api/crawler/tasks/%1/start").arg(taskId)), "{}");
                        });
                        actionLayout->addWidget(startBtn);
                    }
                    if (status == "running") {
                        auto* stopBtn = new QPushButton("Stop");
                        stopBtn->setStyleSheet(
                            "QPushButton { background: #d97706; color: white; border: none; "
                            "border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
                        );
                        connect(stopBtn, &QPushButton::clicked, this, [this, taskId]() {
                            apiManager_->put(apiManager_->createRequest(
                                QString("/api/crawler/tasks/%1/stop").arg(taskId)), "{}");
                        });
                        actionLayout->addWidget(stopBtn);
                    }

                    auto* delBtn = new QPushButton("Del");
                    delBtn->setStyleSheet(
                        "QPushButton { background: #dc2626; color: white; border: none; "
                        "border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
                    );
                    connect(delBtn, &QPushButton::clicked, this, [this, taskId]() {
                        apiManager_->deleteResource(apiManager_->createRequest(
                            QString("/api/crawler/tasks/%1").arg(taskId)));
                        // Refresh after delete
                        QTimer::singleShot(500, this, [this]() {
                            apiManager_->getCrawlerTasks();
                            apiManager_->getCrawlerDashboard();
                        });
                    });
                    actionLayout->addWidget(delBtn);
                    actionLayout->addStretch();

                    taskTable->setCellWidget(i, 5, actionWidget);
                }
            });

        connect(apiManager_, &ApiManager::crawlerDashboardSuccess, this,
            [this, page](const QJsonObject& data) {
                auto updateCard = [page](const QString& name, const QString& value) {
                    auto* card = page->findChild<QWidget*>(name);
                    if (!card) return;
                    auto labels = card->findChildren<QLabel*>();
                    if (!labels.isEmpty()) labels[0]->setText(value);
                };
                int running = data["runningTasks"].toInt(data["running_tasks"].toInt());
                updateCard("Total Tasks", QString::number(data["totalTasks"].toInt(data["total_tasks"].toInt())));
                updateCard("Running", QString::number(running));
                updateCard("Completed", QString::number(data["completedTasks"].toInt(data["completed_tasks"].toInt())));
                updateCard("Failed", QString::number(data["failedTasks"].toInt(data["failed_tasks"].toInt())));

                // Notify when tasks complete
                if (lastRunningCount_ > 0 && running == 0) {
                    if (trayIcon_) {
                        int completed = data["completedTasks"].toInt(data["completed_tasks"].toInt());
                        trayIcon_->showMessage("Crawler Complete",
                            QString("All tasks finished. %1 completed.").arg(completed),
                            QSystemTrayIcon::Information, 3000);
                    }
                }
                lastRunningCount_ = running;
            });

        tabWidget_->addTab(page, "\xf0\x9f\x95\xb7 Crawler");
    }

    // === Tab 4: AI Assistant ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);

        auto* headerRow = new QHBoxLayout();
        auto* header = new QLabel("AI Research Assistant");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        headerRow->addWidget(header);
        headerRow->addStretch();

        // Review selected paper button
        auto* reviewBtn = new QPushButton("Review Paper");
        reviewBtn->setStyleSheet(
            "QPushButton { background: #7c3aed; color: white; border: none; "
            "border-radius: 6px; padding: 6px 16px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #6d28d9; }"
        );
        connect(reviewBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay) return;

            // Use last selected paper from resultView
            if (!resultView_ || resultView_->paperCount() == 0) {
                chatDisplay->append("<i>Select a paper first by searching and clicking a result.</i>");
                return;
            }

            // Get the first paper from current results for review
            auto papers = resultView_->getPapers();
            if (papers.isEmpty()) return;

            const auto& paper = papers.first();
            chatDisplay->append("<b>You:</b> Please review this paper: " + paper.title);

            QJsonObject data;
            data["paperId"] = paper.id;
            data["title"] = paper.title;
            data["abstract"] = paper.abstract;
            apiManager_->aiReview(data);
        });
        headerRow->addWidget(reviewBtn);

        // Export chat button
        auto* exportChatBtn = new QPushButton("Export Chat");
        exportChatBtn->setStyleSheet(
            "QPushButton { background: palette(base); color: palette(text); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 6px 12px; font-size: 12px; }"
            "QPushButton:hover { background: palette(alternate-base); }"
        );
        connect(exportChatBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || chatDisplay->document()->isEmpty()) {
                QMessageBox::information(this, "Export", "No chat to export.");
                return;
            }
            QString fileName = QFileDialog::getSaveFileName(this,
                "Export Chat", "ai_chat.txt", "Text Files (*.txt)");
            if (fileName.isEmpty()) return;
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(chatDisplay->toPlainText().toUtf8());
                file.close();
                statusBar()->showMessage("Chat exported to " + fileName, 3000);
            }
        });
        headerRow->addWidget(exportChatBtn);

        // AI Advanced buttons
        auto* summarizeBtn = new QPushButton("Summarize");
        summarizeBtn->setStyleSheet(
            "QPushButton { background: #0891b2; color: white; border: none; "
            "border-radius: 6px; padding: 6px 12px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #0e7490; }"
        );
        connect(summarizeBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || !resultView_ || resultView_->paperCount() == 0) {
                if (chatDisplay) chatDisplay->append("<i>Search for papers first.</i>");
                return;
            }
            auto papers = resultView_->getPapers();
            QJsonArray paperArray;
            for (int i = 0; i < qMin(5, papers.size()); ++i) {
                QJsonObject p;
                p["id"] = papers[i].id;
                p["title"] = papers[i].title;
                paperArray.append(p);
            }
            QJsonObject data;
            data["papers"] = paperArray;
            apiManager_->aiSummarize(data);
            chatDisplay->append("<b>You:</b> Summarize the current search results");
        });
        headerRow->addWidget(summarizeBtn);

        auto* compareBtn = new QPushButton("Compare");
        compareBtn->setStyleSheet(
            "QPushButton { background: #d97706; color: white; border: none; "
            "border-radius: 6px; padding: 6px 12px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #b45309; }"
        );
        connect(compareBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || !resultView_ || resultView_->paperCount() < 2) {
                if (chatDisplay) chatDisplay->append("<i>Need at least 2 papers to compare.</i>");
                return;
            }
            auto papers = resultView_->getPapers();
            QJsonObject data;
            data["paper1_id"] = papers[0].id;
            data["paper2_id"] = papers[1].id;
            data["paper1_title"] = papers[0].title;
            data["paper2_title"] = papers[1].title;
            apiManager_->aiCompare(data);
            chatDisplay->append("<b>You:</b> Compare the top 2 papers");
        });
        headerRow->addWidget(compareBtn);

        auto* keywordsBtn = new QPushButton("Keywords");
        keywordsBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; "
            "border-radius: 6px; padding: 6px 12px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(keywordsBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || !resultView_ || resultView_->paperCount() == 0) {
                if (chatDisplay) chatDisplay->append("<i>Search for papers first.</i>");
                return;
            }
            auto papers = resultView_->getPapers();
            QJsonObject data;
            data["paper_id"] = papers[0].id;
            data["title"] = papers[0].title;
            data["abstract"] = papers[0].abstract;
            apiManager_->aiKeywords(data);
            chatDisplay->append("<b>You:</b> Extract keywords from top paper");
        });
        headerRow->addWidget(keywordsBtn);

        auto* contributionsBtn = new QPushButton("Contributions");
        contributionsBtn->setStyleSheet(
            "QPushButton { background: #7c3aed; color: white; border: none; "
            "border-radius: 6px; padding: 6px 12px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #6d28d9; }"
        );
        connect(contributionsBtn, &QPushButton::clicked, this, [this, page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (!chatDisplay || !resultView_ || resultView_->paperCount() == 0) {
                if (chatDisplay) chatDisplay->append("<i>Search for papers first.</i>");
                return;
            }
            auto papers = resultView_->getPapers();
            QJsonObject data;
            data["paper_id"] = papers[0].id;
            data["title"] = papers[0].title;
            data["abstract"] = papers[0].abstract;
            apiManager_->aiContributions(data);
            chatDisplay->append("<b>You:</b> Analyze contributions of top paper");
        });
        headerRow->addWidget(contributionsBtn);

        // Clear chat button
        auto* clearChatBtn = new QPushButton("Clear");
        clearChatBtn->setStyleSheet(
            "QPushButton { background: none; border: 1px solid #ef4444; color: #ef4444; "
            "border-radius: 6px; padding: 6px 12px; font-size: 12px; }"
            "QPushButton:hover { background: #fef2f2; }"
        );
        connect(clearChatBtn, &QPushButton::clicked, this, [page]() {
            auto* chatDisplay = page->findChild<QTextEdit*>("aiChatDisplay");
            if (chatDisplay) chatDisplay->clear();
        });
        headerRow->addWidget(clearChatBtn);

        layout->addLayout(headerRow);

        // Quick questions row
        auto* quickRow = new QHBoxLayout();
        quickRow->setSpacing(6);
        auto makeQuickBtn = [](const QString& text) {
            auto* btn = new QPushButton(text);
            btn->setStyleSheet(
                "QPushButton { background: palette(base); border: 1px solid palette(mid); "
                "border-radius: 14px; padding: 4px 14px; font-size: 11px; color: palette(text); }"
                "QPushButton:hover { background: #e0e7ff; }"
            );
            btn->setCursor(Qt::PointingHandCursor);
            return btn;
        };
        for (const auto& q : {"Summarize trends", "Compare methods", "Find gaps", "Suggest keywords"}) {
            auto* btn = makeQuickBtn(q);
            connect(btn, &QPushButton::clicked, this, [this, btn]() {
                // Forward to chat input
                auto* page = btn->parentWidget();
                while (page && !page->inherits("QWidget")) page = page->parentWidget();
                auto* chatInput = page ? page->findChild<QLineEdit*>() : nullptr;
                auto* chatDisplay = page ? page->findChild<QTextEdit*>("aiChatDisplay") : nullptr;
                if (chatInput && chatDisplay) {
                    chatDisplay->append("<b>You:</b> " + btn->text());
                    QJsonObject data;
                    data["message"] = btn->text();
                    data["userId"] = 1;
                    apiManager_->aiChat(data);
                }
            });
            quickRow->addWidget(btn);
        }
        layout->addLayout(quickRow);

        // AI Copilot section
        {
            auto* copilotBar = new QHBoxLayout();
            copilotBar->setSpacing(8);

            auto* copilotLabel = new QLabel("AI Copilot:");
            copilotLabel->setStyleSheet("font-weight: bold; color: palette(mid); font-size: 12px;");
            copilotBar->addWidget(copilotLabel);

            auto* litReviewBtn = new QPushButton("Literature Review");
            litReviewBtn->setStyleSheet(
                "QPushButton { background: #7c3aed; color: white; border: none; border-radius: 6px; "
                "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
                "QPushButton:hover { background: #6d28d9; }"
            );
            connect(litReviewBtn, &QPushButton::clicked, this, [this]() {
                QDialog dlg(this);
                dlg.setWindowTitle("Generate Literature Review");
                dlg.setMinimumWidth(500);
                auto* form = new QVBoxLayout(&dlg);
                auto* topicEdit = new QLineEdit();
                topicEdit->setPlaceholderText("Research topic (e.g., transformer architectures)");
                topicEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
                form->addWidget(topicEdit);
                auto* focusEdit = new QTextEdit();
                focusEdit->setPlaceholderText("Focus areas (optional, one per line)");
                focusEdit->setMaximumHeight(80);
                focusEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
                form->addWidget(focusEdit);
                auto* btnRow = new QHBoxLayout();
                auto* genBtn = new QPushButton("Generate");
                genBtn->setStyleSheet("QPushButton { background: #7c3aed; color: white; border: none; border-radius: 6px; padding: 8px 24px; font-weight: bold; }");
                auto* cancelBtn = new QPushButton("Cancel");
                cancelBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 8px 20px; }");
                connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
                connect(genBtn, &QPushButton::clicked, this, [this, &dlg, topicEdit, focusEdit]() {
                    if (topicEdit->text().trimmed().isEmpty()) return;
                    QJsonObject data;
                    data["topic"] = topicEdit->text().trimmed();
                    data["focus_areas"] = focusEdit->toPlainText();
                    apiManager_->generateLiteratureReview(data);
                    dlg.accept();
                    statusBar()->showMessage("Generating literature review...", 5000);
                });
                btnRow->addStretch();
                btnRow->addWidget(cancelBtn);
                btnRow->addWidget(genBtn);
                form->addLayout(btnRow);
                dlg.exec();
            });
            copilotBar->addWidget(litReviewBtn);

            auto* researchPlanBtn = new QPushButton("Research Plan");
            researchPlanBtn->setStyleSheet(
                "QPushButton { background: #0891b2; color: white; border: none; border-radius: 6px; "
                "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
                "QPushButton:hover { background: #0e7490; }"
            );
            connect(researchPlanBtn, &QPushButton::clicked, this, [this]() {
                QDialog dlg(this);
                dlg.setWindowTitle("Generate Research Plan");
                dlg.setMinimumWidth(500);
                auto* form = new QVBoxLayout(&dlg);
                auto* topicEdit = new QLineEdit();
                topicEdit->setPlaceholderText("Research topic");
                topicEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
                form->addWidget(topicEdit);
                auto* goalEdit = new QTextEdit();
                goalEdit->setPlaceholderText("Research goals (one per line)");
                goalEdit->setMaximumHeight(80);
                goalEdit->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
                form->addWidget(goalEdit);
                auto* btnRow = new QHBoxLayout();
                auto* genBtn = new QPushButton("Generate");
                genBtn->setStyleSheet("QPushButton { background: #0891b2; color: white; border: none; border-radius: 6px; padding: 8px 24px; font-weight: bold; }");
                auto* cancelBtn = new QPushButton("Cancel");
                cancelBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 8px 20px; }");
                connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
                connect(genBtn, &QPushButton::clicked, this, [this, &dlg, topicEdit, goalEdit]() {
                    if (topicEdit->text().trimmed().isEmpty()) return;
                    QJsonObject data;
                    data["topic"] = topicEdit->text().trimmed();
                    data["goals"] = goalEdit->toPlainText();
                    apiManager_->generateResearchPlan(data);
                    dlg.accept();
                    statusBar()->showMessage("Generating research plan...", 5000);
                });
                btnRow->addStretch();
                btnRow->addWidget(cancelBtn);
                btnRow->addWidget(genBtn);
                form->addLayout(btnRow);
                dlg.exec();
            });
            copilotBar->addWidget(researchPlanBtn);

            auto* myReviewsBtn = new QPushButton("My Reviews");
            myReviewsBtn->setStyleSheet(
                "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
                "border-radius: 6px; padding: 4px 12px; font-size: 11px; }"
                "QPushButton:hover { background: palette(light); }"
            );
            connect(myReviewsBtn, &QPushButton::clicked, this, [this]() {
                apiManager_->getLiteratureReviews();
                apiManager_->getResearchPlans();
                statusBar()->showMessage("Loading your reviews and plans...", 3000);
            });
            copilotBar->addWidget(myReviewsBtn);

            auto* aiStatusBtn = new QPushButton("AI Status");
            aiStatusBtn->setStyleSheet(myReviewsBtn->styleSheet());
            connect(aiStatusBtn, &QPushButton::clicked, this, [this]() {
                apiManager_->getAiStatus();
            });
            copilotBar->addWidget(aiStatusBtn);

            copilotBar->addStretch();
            layout->addLayout(copilotBar);
        }

        auto* chatDisplay = new QTextEdit();
        chatDisplay->setObjectName("aiChatDisplay");
        chatDisplay->setReadOnly(true);
        chatDisplay->setStyleSheet(
            "QTextEdit { background: palette(base); border: 1px solid palette(mid); "
            "border-radius: 8px; padding: 12px; font-size: 14px; }"
        );
        chatDisplay->setPlaceholderText("AI responses will appear here...");
        layout->addWidget(chatDisplay, 1);

        auto* inputRow = new QHBoxLayout();
        auto* chatInput = new QLineEdit();
        chatInput->setPlaceholderText("Ask about papers, research topics...");
        chatInput->setStyleSheet("QLineEdit { padding: 10px; border: 1px solid palette(mid); border-radius: 8px; }");
        inputRow->addWidget(chatInput, 1);

        auto* sendBtn = new QPushButton("Send");
        sendBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; "
            "border-radius: 8px; padding: 10px 24px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        inputRow->addWidget(sendBtn);
        layout->addLayout(inputRow);

        connect(sendBtn, &QPushButton::clicked, this, [this, chatInput, chatDisplay]() {
            QString msg = chatInput->text().trimmed();
            if (msg.isEmpty()) return;
            chatDisplay->append("<b>You:</b> " + msg);
            chatInput->clear();
            QJsonObject data;
            data["message"] = msg;
            data["userId"] = 1;
            apiManager_->aiChat(data);
        });
        connect(chatInput, &QLineEdit::returnPressed, sendBtn, &QPushButton::click);

        connect(apiManager_, &ApiManager::aiChatSuccess, this, [chatDisplay](const QString& response) {
            chatDisplay->append("<b>AI:</b> " + response);
        });

        connect(apiManager_, &ApiManager::aiReviewSuccess, this, [chatDisplay](const QJsonObject& result) {
            QString review = result["review"].toString(result["summary"].toString("Review completed."));
            chatDisplay->append("<b>AI Review:</b> " + review);
        });

        tabWidget_->addTab(page, "\xf0\x9f\xa4\x96 AI");
    }

    // === Tab 5: Statistics ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(16);

        auto* header = new QLabel("Statistics");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        layout->addWidget(header);

        // Summary cards row
        auto* cardsRow = new QHBoxLayout();
        cardsRow->setSpacing(12);

        auto makeStatCard = [](const QString& name, const QString& icon, const QString& color) {
            auto* card = new QWidget();
            card->setObjectName(name);
            card->setStyleSheet(
                QString("QWidget { background: %1; border-radius: 8px; padding: 16px; }").arg(color)
            );
            auto* l = new QVBoxLayout(card);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(4);
            auto* iconLabel = new QLabel(icon);
            iconLabel->setStyleSheet("font-size: 20px;");
            l->addWidget(iconLabel);
            auto* v = new QLabel("--");
            v->setObjectName(name + "Value");
            v->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
            l->addWidget(v);
            auto* t = new QLabel(name);
            t->setStyleSheet("font-size: 11px; color: rgba(255,255,255,0.8); font-weight: 500;");
            l->addWidget(t);
            return card;
        };

        cardsRow->addWidget(makeStatCard("Papers", "📄", "#4f46e5"));
        cardsRow->addWidget(makeStatCard("Journals", "📚", "#059669"));
        cardsRow->addWidget(makeStatCard("Authors", "👥", "#0891b2"));
        cardsRow->addWidget(makeStatCard("Crawled", "🕷️", "#d97706"));
        layout->addLayout(cardsRow);

        // Year distribution table
        auto* yearHeader = new QLabel("Year Distribution");
        yearHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(yearHeader);

        auto* yearTable = new QTableWidget(0, 3);
        yearTable->setObjectName("statsYearTable");
        yearTable->setHorizontalHeaderLabels({"Year", "Count", "Percentage"});
        yearTable->horizontalHeader()->setStretchLastSection(true);
        yearTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        yearTable->setAlternatingRowColors(true);
        yearTable->setMaximumHeight(200);
        layout->addWidget(yearTable);

        // Top journals table
        auto* journalHeader = new QLabel("Top Journals");
        journalHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(journalHeader);

        auto* journalTable = new QTableWidget(0, 3);
        journalTable->setObjectName("statsJournalTable");
        journalTable->setHorizontalHeaderLabels({"Journal", "Papers", "Level"});
        journalTable->horizontalHeader()->setStretchLastSection(true);
        journalTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        journalTable->setAlternatingRowColors(true);
        layout->addWidget(journalTable, 1);

        // Detailed stats buttons
        auto* detailStatsBar = new QHBoxLayout();
        auto* sysStatsBtn = new QPushButton("System Stats");
        sysStatsBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(sysStatsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getSystemStats();
        });
        auto* resStatsBtn = new QPushButton("Resource Stats");
        resStatsBtn->setStyleSheet(
            "QPushButton { background: #0891b2; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #0e7490; }"
        );
        connect(resStatsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getResourceStats();
        });
        auto* perfStatsBtn = new QPushButton("Performance Stats");
        perfStatsBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; border-radius: 6px; "
            "padding: 6px 14px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #047857; }"
        );
        connect(perfStatsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getPerformanceStats();
        });
        detailStatsBar->addWidget(sysStatsBtn);
        detailStatsBar->addWidget(resStatsBtn);
        detailStatsBar->addWidget(perfStatsBtn);
        detailStatsBar->addStretch();
        layout->addLayout(detailStatsBar);

        connect(apiManager_, &ApiManager::statsSuccess, this,
            [page](const QJsonObject& stats) {
                // Summary cards
                int totalPapers = stats["total_papers"].toInt(stats["totalPapers"].toInt(0));
                int totalJournals = stats["total_journals"].toInt(stats["totalJournals"].toInt(0));
                int totalAuthors = stats["total_authors"].toInt(stats["totalAuthors"].toInt(0));
                int crawled = stats["crawled_count"].toInt(stats["crawledCount"].toInt(0));

                auto updateVal = [page](const QString& name, const QString& val) {
                    auto* label = page->findChild<QLabel*>(name + "Value");
                    if (label) label->setText(val);
                };
                updateVal("Papers", QString::number(totalPapers));
                updateVal("Journals", QString::number(totalJournals));
                updateVal("Authors", QString::number(totalAuthors));
                updateVal("Crawled", QString::number(crawled));

                // Year distribution
                auto* yearTable = page->findChild<QTableWidget*>("statsYearTable");
                if (yearTable) {
                    QJsonObject yearDist = stats["year_distribution"].toObject(stats["yearDistribution"].toObject());
                    yearTable->setRowCount(yearDist.size());
                    int row = 0;
                    for (auto it = yearDist.begin(); it != yearDist.end(); ++it, ++row) {
                        yearTable->setItem(row, 0, new QTableWidgetItem(it.key()));
                        yearTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value().toInt())));
                        double pct = totalPapers > 0 ? (it.value().toInt() * 100.0 / totalPapers) : 0;
                        yearTable->setItem(row, 2, new QTableWidgetItem(QString("%1%").arg(pct, 0, 'f', 1)));
                    }
                }

                // Top journals
                auto* journalTable = page->findChild<QTableWidget*>("statsJournalTable");
                if (journalTable) {
                    QJsonArray topJournals = stats["top_journals"].toArray(stats["topJournals"].toArray());
                    journalTable->setRowCount(topJournals.size());
                    for (int i = 0; i < topJournals.size(); ++i) {
                        auto j = topJournals[i].toObject();
                        journalTable->setItem(i, 0, new QTableWidgetItem(j["name"].toString(j["journal"].toString())));
                        journalTable->setItem(i, 1, new QTableWidgetItem(QString::number(j["count"].toInt(j["paper_count"].toInt()))));
                        journalTable->setItem(i, 2, new QTableWidgetItem(j["level"].toString()));
                    }
                }
            });

        tabWidget_->addTab(page, "\xf0\x9f\x93\x8a Statistics");
    }

    // === Tab 6: Recommendations ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 16, 20, 16);
        layout->setSpacing(16);

        // Header
        auto* header = new QLabel("\xf0\x9f\x92\xa1 Recommendations");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        layout->addWidget(header);

        // Recommendations list with scroll
        auto* scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setStyleSheet("QScrollArea { border: none; background: transparent; }");
        auto* recContainer = new QWidget();
        recContainer->setObjectName("recContainer");
        auto* recLayout = new QVBoxLayout(recContainer);
        recLayout->setSpacing(12);
        recLayout->setObjectName("recListLayout");

        auto* recLoading = new QLabel("Loading recommendations...");
        recLoading->setObjectName("recLoadingLabel");
        recLoading->setAlignment(Qt::AlignCenter);
        recLoading->setStyleSheet("color: palette(mid); font-size: 13px; padding: 40px;");
        recLayout->addWidget(recLoading);
        recLayout->addStretch();

        scroll->setWidget(recContainer);
        layout->addWidget(scroll, 1);

        // Bottom bar: trending + refresh
        auto* bottomBar = new QHBoxLayout();
        auto* trendingBtn = new QPushButton("\xf0\x9f\x94\xa5 Trending Papers");
        trendingBtn->setObjectName("trendingBtn");
        trendingBtn->setStyleSheet(
            "QPushButton { background: #f59e0b; color: white; border: none; border-radius: 8px; "
            "padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #d97706; }"
        );
        auto* refreshRecBtn = new QPushButton("\xf0\x9f\x94\x84 Refresh");
        refreshRecBtn->setObjectName("refreshRecBtn");
        refreshRecBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 8px; padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        bottomBar->addWidget(trendingBtn);
        bottomBar->addStretch();
        bottomBar->addWidget(refreshRecBtn);
        layout->addLayout(bottomBar);

        // Signals
        connect(trendingBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getTrendingPapers();
            statusBar()->showMessage("Loading trending papers...", 3000);
        });
        connect(refreshRecBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getPaperRecommendations();
            statusBar()->showMessage("Refreshing recommendations...", 3000);
        });

        tabWidget_->addTab(page, "\xf0\x9f\x92\xa1 Recommend");
    }

    // === Tab 7: Admin ===
    {
        auto* page = new QWidget();
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 16, 20, 16);
        layout->setSpacing(16);

        // Header
        auto* header = new QLabel("\xf0\x9f\x94\xa7 Admin Panel");
        header->setStyleSheet("font-size: 20px; font-weight: bold; color: palette(text);");
        layout->addWidget(header);

        // Summary cards row
        auto* cardsRow = new QHBoxLayout();
        cardsRow->setSpacing(12);

        auto makeAdminCard = [](const QString& name, const QString& icon, const QString& color) {
            auto* card = new QWidget();
            card->setObjectName(name);
            card->setStyleSheet(
                QString("QWidget { background: %1; border-radius: 8px; padding: 16px; }").arg(color)
            );
            auto* l = new QVBoxLayout(card);
            l->setContentsMargins(0, 0, 0, 0);
            l->setSpacing(4);
            auto* iconLabel = new QLabel(icon);
            iconLabel->setStyleSheet("font-size: 20px;");
            l->addWidget(iconLabel);
            auto* v = new QLabel("--");
            v->setObjectName(name + "Value");
            v->setStyleSheet("font-size: 24px; font-weight: bold; color: white;");
            l->addWidget(v);
            auto* t = new QLabel(name);
            t->setStyleSheet("font-size: 11px; color: rgba(255,255,255,0.8); font-weight: 500;");
            l->addWidget(t);
            return card;
        };
        cardsRow->addWidget(makeAdminCard("Users", "\xf0\x9f\x91\xa4", "#6366f1"));
        cardsRow->addWidget(makeAdminCard("Modules", "\xf0\x9f\xa7\xa9", "#0891b2"));
        cardsRow->addWidget(makeAdminCard("Uptime", "\xe2\x8f\xb1\xef\xb8\x8f", "#059669"));
        cardsRow->addWidget(makeAdminCard("Requests", "\xf0\x9f\x93\xa1", "#d97706"));
        layout->addLayout(cardsRow);

        // Users table
        auto* usersHeader = new QLabel("Users");
        usersHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(usersHeader);

        auto* usersTable = new QTableWidget(0, 5);
        usersTable->setObjectName("adminUsersTable");
        usersTable->setHorizontalHeaderLabels({"ID", "Username", "Email", "Role", "Status"});
        usersTable->horizontalHeader()->setStretchLastSection(true);
        usersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        usersTable->setAlternatingRowColors(true);
        usersTable->setMaximumHeight(250);
        usersTable->setStyleSheet(
            "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
            "QHeaderView::section { background: palette(window); color: palette(text); "
            "font-weight: bold; padding: 6px; border: none; border-bottom: 1px solid palette(mid); }"
            "QTableWidget::item { padding: 4px; color: palette(text); }"
            "QTableWidget::item:alternate { background: palette(alternate-base); }"
        );
        layout->addWidget(usersTable);

        // Modules section
        auto* modulesHeader = new QLabel("Modules");
        modulesHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(modulesHeader);

        auto* modulesTable = new QTableWidget(0, 4);
        modulesTable->setObjectName("adminModulesTable");
        modulesTable->setHorizontalHeaderLabels({"Name", "Version", "Status", "Action"});
        modulesTable->horizontalHeader()->setStretchLastSection(true);
        modulesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        modulesTable->setAlternatingRowColors(true);
        modulesTable->setMaximumHeight(200);
        modulesTable->setStyleSheet(usersTable->styleSheet());
        layout->addWidget(modulesTable);

        // System monitor section
        auto* monitorHeader = new QLabel("System Monitor");
        monitorHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
        layout->addWidget(monitorHeader);

        auto* monitorInfo = new QLabel("CPU: -- | Memory: -- | Disk: --");
        monitorInfo->setObjectName("adminMonitorInfo");
        monitorInfo->setStyleSheet(
            "padding: 12px; background: palette(base); border: 1px solid palette(mid); "
            "border-radius: 8px; color: palette(text); font-size: 13px;"
        );
        layout->addWidget(monitorInfo);

        // Bottom actions
        auto* adminActions = new QHBoxLayout();
        auto* refreshAdminBtn = new QPushButton("\xf0\x9f\x94\x84 Refresh All");
        refreshAdminBtn->setObjectName("refreshAdminBtn");
        refreshAdminBtn->setStyleSheet(
            "QPushButton { background: #6366f1; color: white; border: none; border-radius: 8px; "
            "padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: #4f46e5; }"
        );
        auto* loginHistoryBtn = new QPushButton("\xf0\x9f\x93\x8b Login History");
        loginHistoryBtn->setObjectName("loginHistoryBtn");
        loginHistoryBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 8px; padding: 8px 20px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        auto* perfMetricsBtn = new QPushButton("\xf0\x9f\x93\x8a Performance");
        perfMetricsBtn->setObjectName("perfMetricsBtn");
        perfMetricsBtn->setStyleSheet(loginHistoryBtn->styleSheet());
        adminActions->addWidget(refreshAdminBtn);
        adminActions->addStretch();
        adminActions->addWidget(loginHistoryBtn);
        adminActions->addWidget(perfMetricsBtn);
        layout->addLayout(adminActions);

        // Admin button signals
        connect(refreshAdminBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getAdminDashboard();
            apiManager_->getAdminUsers();
            apiManager_->getAdminModules();
            apiManager_->getSystemMonitor();
            statusBar()->showMessage("Refreshing admin data...", 3000);
        });
        connect(loginHistoryBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getLoginHistory();
            statusBar()->showMessage("Loading login history...", 3000);
        });
        connect(perfMetricsBtn, &QPushButton::clicked, this, [this]() {
            apiManager_->getPerformanceMetrics();
            statusBar()->showMessage("Loading performance metrics...", 3000);
        });

        tabWidget_->addTab(page, "\xf0\x9f\x94\xa7 Admin");
    }

    // === Tab 8: LaTeX Editor ===
    {
        auto* page = new QWidget();
        page->setObjectName("latexEditorPage");
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        latexEditor_ = new LatexEditorWidget(apiManager_, this);
        layout->addWidget(latexEditor_);

        connect(latexEditor_, &LatexEditorWidget::statusMessage, this, [this](const QString& msg) {
            statusBar()->showMessage(msg, 3000);
        });

        tabWidget_->addTab(page, "\xf0\x9f\x93\x9d LaTeX");
    }

    // Update tab position to bottom with scroll buttons for 8 tabs
    tabWidget_->setUsesScrollButtons(true);
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

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (themeButton_) {
        themeButton_->move(width() - 70, 80);
    }
}

void MainWindow::populateRecommendations(const QJsonObject& data) {
    auto* container = findChild<QWidget*>("recContainer");
    if (!container) return;
    auto* layout = container->findChild<QVBoxLayout*>("recListLayout");
    if (!layout) return;

    // Clear old content
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    QJsonArray papers = data["papers"].toArray(data["recommendations"].toArray());
    QString source = data["source"].toString("recommendations");

    if (papers.isEmpty()) {
        auto* emptyLabel = new QLabel("No recommendations available yet. Try searching for papers first!");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: palette(mid); font-size: 14px; padding: 60px;");
        layout->addWidget(emptyLabel);
        layout->addStretch();
        return;
    }

    // Source header
    QString sourceText = source == "trending" ? "\xf0\x9f\x94\xa5 Trending Papers"
                       : source == "similar" ? "\xf0\x9f\x94\x97 Similar Papers"
                       : "\xf0\x9f\x92\xa1 Recommended for You";
    auto* sourceLabel = new QLabel(sourceText);
    sourceLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text); padding: 8px 0;");
    layout->addWidget(sourceLabel);

    for (const auto& val : papers) {
        auto p = val.toObject();
        auto* card = new QWidget();
        card->setStyleSheet(
            "QWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 10px; }"
        );
        card->setCursor(Qt::PointingHandCursor);

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 12, 16, 12);
        cardLayout->setSpacing(6);

        // Title
        QString title = p["title"].toString(p["paper_title"].toString());
        auto* titleLabel = new QLabel(title.left(120));
        titleLabel->setWordWrap(true);
        titleLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: palette(text);");
        cardLayout->addWidget(titleLabel);

        // Meta row
        auto* metaLayout = new QHBoxLayout();
        QString journal = p["journal"].toString(p["journal_name"].toString());
        QString year = p["year"].toString(p["publication_year"].toString());
        QString score = p["score"].toString(p["relevance_score"].toString());

        auto* metaLabel = new QLabel(journal + (year.isEmpty() ? "" : " | " + year));
        metaLabel->setStyleSheet("color: palette(mid); font-size: 11px;");
        metaLayout->addWidget(metaLabel);
        metaLayout->addStretch();

        if (!score.isEmpty()) {
            auto* scoreLabel = new QLabel("\xe2\xad\x90 " + score.left(5));
            scoreLabel->setStyleSheet("color: #f59e0b; font-weight: bold; font-size: 11px;");
            metaLayout->addWidget(scoreLabel);
        }
        cardLayout->addLayout(metaLayout);

        // Authors
        QString authors = p["authors"].toString();
        if (!authors.isEmpty()) {
            auto* authorLabel = new QLabel(authors.left(100));
            authorLabel->setStyleSheet("color: palette(mid); font-size: 10px;");
            cardLayout->addWidget(authorLabel);
        }

        // Action buttons
        auto* actions = new QHBoxLayout();
        int paperId = p["id"].toInt(p["paper_id"].toInt(0));

        auto* viewBtn = new QPushButton("View Details");
        viewBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 6px; "
            "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(viewBtn, &QPushButton::clicked, this, [this, paperId]() {
            onPaperSelected(paperId);
        });
        actions->addWidget(viewBtn);

        auto* similarBtn = new QPushButton("\xf0\x9f\x94\x97 Similar");
        similarBtn->setStyleSheet(
            "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
            "border-radius: 6px; padding: 4px 12px; font-size: 11px; }"
            "QPushButton:hover { background: palette(light); }"
        );
        connect(similarBtn, &QPushButton::clicked, this, [this, paperId]() {
            apiManager_->getSimilarPapers(paperId);
            statusBar()->showMessage("Finding similar papers...", 3000);
        });
        actions->addWidget(similarBtn);

        auto* feedbackBtn = new QPushButton("\xf0\x9f\x91\x8d");
        feedbackBtn->setStyleSheet(
            "QPushButton { background: transparent; border: 1px solid palette(mid); border-radius: 6px; "
            "padding: 4px 8px; font-size: 12px; }"
            "QPushButton:hover { background: #dcfce7; }"
        );
        connect(feedbackBtn, &QPushButton::clicked, this, [this, paperId]() {
            QJsonObject feedback;
            feedback["paper_id"] = paperId;
            feedback["action"] = "positive";
            apiManager_->submitRecommendationFeedback(feedback);
        });
        actions->addWidget(feedbackBtn);

        auto* whyBtn = new QPushButton("Why?");
        whyBtn->setStyleSheet(
            "QPushButton { background: transparent; border: 1px solid palette(mid); border-radius: 6px; "
            "padding: 4px 8px; font-size: 11px; color: palette(text); }"
            "QPushButton:hover { background: palette(alternate-base); }"
        );
        connect(whyBtn, &QPushButton::clicked, this, [this, paperId]() {
            apiManager_->getRecommendationExplanation(paperId);
        });
        actions->addWidget(whyBtn);

        actions->addStretch();
        cardLayout->addLayout(actions);

        // Click on card to view details
        connect(card, &QWidget::mousePressEvent, this, [this, paperId](QMouseEvent*) {
            onPaperSelected(paperId);
        });

        layout->addWidget(card);
    }

    layout->addStretch();
    int total = data["total"].toInt(papers.size());
    statusBar()->showMessage(QString("Loaded %1 papers").arg(total), 3000);
}

void MainWindow::populateAdminDashboard(const QJsonObject& data) {
    auto updateVal = [this](const QString& name, const QString& val) {
        auto* label = findChild<QLabel*>(name + "Value");
        if (label) label->setText(val);
    };
    updateVal("Users", QString::number(data["total_users"].toInt(data["user_count"].toInt(0))));
    updateVal("Modules", QString::number(data["active_modules"].toInt(data["module_count"].toInt(0))));
    updateVal("Uptime", data["uptime"].toString(data["uptime_human"].toString("--")));
    updateVal("Requests", QString::number(data["total_requests"].toInt(data["request_count"].toInt(0))));
}

void MainWindow::populateAdminUsers(const QJsonObject& data) {
    auto* table = findChild<QTableWidget*>("adminUsersTable");
    if (!table) return;

    QJsonArray users = data["users"].toArray(data["data"].toArray());
    table->setRowCount(users.size());
    for (int i = 0; i < users.size(); ++i) {
        auto u = users[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(u["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(u["username"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(u["email"].toString()));
        table->setItem(i, 3, new QTableWidgetItem(u["role"].toString()));
        table->setItem(i, 4, new QTableWidgetItem(u["status"].toString(u["is_active"].toBool() ? "Active" : "Inactive")));
    }
}

void MainWindow::populateAdminModules(const QJsonObject& data) {
    auto* table = findChild<QTableWidget*>("adminModulesTable");
    if (!table) return;

    QJsonArray modules = data["modules"].toArray(data["data"].toArray());
    table->setRowCount(modules.size());
    for (int i = 0; i < modules.size(); ++i) {
        auto m = modules[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(m["name"].toString()));
        table->setItem(i, 1, new QTableWidgetItem(m["version"].toString("--")));

        bool enabled = m["enabled"].toBool(m["is_active"].toBool(true));
        auto* statusItem = new QTableWidgetItem(enabled ? "Enabled" : "Disabled");
        statusItem->setForeground(enabled ? QBrush(QColor("#059669")) : QBrush(QColor("#dc2626")));
        table->setItem(i, 2, statusItem);

        // Toggle button
        auto* toggleBtn = new QPushButton(enabled ? "Disable" : "Enable");
        toggleBtn->setStyleSheet(
            enabled
            ? "QPushButton { background: #dc2626; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 11px; }"
            : "QPushButton { background: #059669; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 11px; }"
        );
        QString modName = m["name"].toString();
        bool newState = !enabled;
        connect(toggleBtn, &QPushButton::clicked, this, [this, modName, newState]() {
            apiManager_->toggleModule(modName, newState);
            statusBar()->showMessage(QString("Toggling module: %1...").arg(modName), 3000);
        });
        table->setCellWidget(i, 3, toggleBtn);
    }
}

void MainWindow::populateAdminMonitor(const QJsonObject& data) {
    auto* label = findChild<QLabel*>("adminMonitorInfo");
    if (!label) return;

    QString cpu = data["cpu_usage"].toString(data["cpu"].toString("--"));
    QString mem = data["memory_usage"].toString(data["memory"].toString("--"));
    QString disk = data["disk_usage"].toString(data["disk"].toString("--"));
    QString connections = data["active_connections"].toString(data["connections"].toString("--"));

    label->setText(
        QString("CPU: %1% | Memory: %2% | Disk: %3% | Connections: %4")
            .arg(cpu, mem, disk, connections)
    );
}

void MainWindow::populatePerformanceMetrics(const QJsonObject& data) {
    // Show as dialog for better visibility
    QDialog dlg(this);
    dlg.setWindowTitle("Performance Metrics");
    dlg.setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(&dlg);

    auto* info = new QTextEdit();
    info->setReadOnly(true);
    info->setStyleSheet(
        "QTextEdit { background: palette(base); color: palette(text); border: 1px solid palette(mid); "
        "border-radius: 8px; padding: 12px; font-family: monospace; font-size: 12px; }"
    );

    QStringList lines;
    lines << "=== Performance Metrics ===";
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.value().isObject()) {
            lines << QString("\n%1:").arg(it.key());
            auto sub = it.value().toObject();
            for (auto sit = sub.begin(); sit != sub.end(); ++sit) {
                lines << QString("  %1: %2").arg(sit.key(), sit.value().toVariant().toString());
            }
        } else if (it.value().isArray()) {
            lines << QString("\n%1: [%2 items]").arg(it.key()).arg(it.value().toArray().size());
        } else {
            lines << QString("%1: %2").arg(it.key(), it.value().toVariant().toString());
        }
    }
    info->setPlainText(lines.join("\n"));
    layout->addWidget(info);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::showLoginHistory(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("Login History");
    dlg.setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray entries = data["logins"].toArray(data["data"].toArray(data["entries"].toArray()));
    auto* table = new QTableWidget(entries.size(), 4);
    table->setHorizontalHeaderLabels({"Time", "User", "IP", "Status"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < entries.size(); ++i) {
        auto e = entries[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(e["timestamp"].toString(e["time"].toString())));
        table->setItem(i, 1, new QTableWidgetItem(e["username"].toString(e["user"].toString())));
        table->setItem(i, 2, new QTableWidgetItem(e["ip_address"].toString(e["ip"].toString())));
        table->setItem(i, 3, new QTableWidgetItem(e["status"].toString(e["action"].toString())));
    }
    layout->addWidget(table);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::populateCrawlerSchedules(const QJsonObject& data) {
    auto* table = findChild<QTableWidget*>("crawlerSchedulesTable");
    if (!table) return;

    QJsonArray schedules = data["schedules"].toArray(data["data"].toArray());
    table->setRowCount(schedules.size());
    for (int i = 0; i < schedules.size(); ++i) {
        auto s = schedules[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(s["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(s["cron"].toString(s["schedule"].toString())));
        table->setItem(i, 2, new QTableWidgetItem(
            s["template_name"].toString(s["template_id"].toString())));

        auto* actions = new QWidget();
        auto* al = new QHBoxLayout(actions);
        al->setContentsMargins(4, 2, 4, 2);
        al->setSpacing(4);
        int sid = s["id"].toInt();

        auto* triggerBtn = new QPushButton("Run");
        triggerBtn->setStyleSheet(
            "QPushButton { background: #059669; color: white; border: none; border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
        );
        connect(triggerBtn, &QPushButton::clicked, this, [this, sid]() {
            apiManager_->triggerCrawlerSchedule(sid);
            statusBar()->showMessage(QString("Triggered schedule %1").arg(sid), 3000);
        });
        al->addWidget(triggerBtn);
        al->addStretch();
        table->setCellWidget(i, 3, actions);
    }
}

void MainWindow::populateCrawlerTemplates(const QJsonObject& data) {
    auto* table = findChild<QTableWidget*>("crawlerTemplatesTable");
    if (!table) return;

    QJsonArray templates = data["templates"].toArray(data["data"].toArray());
    table->setRowCount(templates.size());
    for (int i = 0; i < templates.size(); ++i) {
        auto t = templates[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(t["name"].toString()));
        table->setItem(i, 1, new QTableWidgetItem(t["source"].toString()));
        table->setItem(i, 2, new QTableWidgetItem(t["url_pattern"].toString().left(40)));

        auto* actions = new QWidget();
        auto* al = new QHBoxLayout(actions);
        al->setContentsMargins(4, 2, 4, 2);
        al->setSpacing(4);
        int tid = t["id"].toInt();

        auto* testBtn = new QPushButton("Test");
        testBtn->setStyleSheet(
            "QPushButton { background: #0891b2; color: white; border: none; border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
        );
        connect(testBtn, &QPushButton::clicked, this, [this, tid]() {
            apiManager_->testCrawlerTemplate(tid);
            statusBar()->showMessage(QString("Testing template %1...").arg(tid), 3000);
        });
        al->addWidget(testBtn);

        auto* delBtn = new QPushButton("Del");
        delBtn->setStyleSheet(
            "QPushButton { background: #dc2626; color: white; border: none; border-radius: 3px; padding: 2px 8px; font-size: 10px; }"
        );
        connect(delBtn, &QPushButton::clicked, this, [this, tid]() {
            apiManager_->deleteCrawlerTemplate(tid);
            QTimer::singleShot(500, this, [this]() { apiManager_->getCrawlerTemplates(); });
        });
        al->addWidget(delBtn);
        al->addStretch();
        table->setCellWidget(i, 3, actions);
    }
}

void MainWindow::populateSavedSearches(const QJsonObject& data) {
    auto* container = findChild<QWidget*>("savedSearchesList");
    if (!container) return;
    auto* layout = container->layout();
    if (!layout) return;

    // Clear old items (keep the stretch at end)
    QLayoutItem* item;
    while (layout->count() > 0) {
        item = layout->takeAt(0);
        delete item->widget();
        delete item;
    }

    QJsonArray searches = data["searches"].toArray(data["data"].toArray(data["saved_searches"].toArray()));
    for (const auto& val : searches) {
        auto s = val.toObject();
        QString query = s["query"].toString(s["keyword"].toString());
        QString name = s["name"].toString(query.left(20));

        auto* chip = new QPushButton(name);
        chip->setStyleSheet(
            "QPushButton { background: #e0e7ff; color: #4338ca; border: none; border-radius: 12px; "
            "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #c7d2fe; }"
        );
        chip->setCursor(Qt::PointingHandCursor);
        connect(chip, &QPushButton::clicked, this, [this, query]() {
            onSearch(query);
            searchWidget_->setText(query);
        });
        layout->addWidget(chip);
    }
    layout->addStretch();
}

void MainWindow::populateTrendingSearches(const QJsonObject& data) {
    auto* container = findChild<QWidget*>("trendingSearchesList");
    if (!container) return;
    auto* layout = container->layout();
    if (!layout) return;

    QLayoutItem* item;
    while (layout->count() > 0) {
        item = layout->takeAt(0);
        delete item->widget();
        delete item;
    }

    QJsonArray trending = data["trending"].toArray(data["data"].toArray(data["trending_searches"].toArray()));
    for (const auto& val : trending) {
        QString query;
        if (val.isObject()) {
            query = val.toObject()["query"].toString(val.toObject()["keyword"].toString());
        } else {
            query = val.toString();
        }
        if (query.isEmpty()) continue;

        auto* chip = new QPushButton(query.left(25));
        chip->setStyleSheet(
            "QPushButton { background: #fef3c7; color: #92400e; border: none; border-radius: 12px; "
            "padding: 4px 12px; font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { background: #fde68a; }"
        );
        chip->setCursor(Qt::PointingHandCursor);
        connect(chip, &QPushButton::clicked, this, [this, query]() {
            onSearch(query);
            searchWidget_->setText(query);
        });
        layout->addWidget(chip);
    }
    layout->addStretch();
}

void MainWindow::showStatsDialog(const QString& title, const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle(title);
    dlg.setMinimumSize(700, 500);
    auto* layout = new QVBoxLayout(&dlg);

    // Summary cards at top
    auto* cardsRow = new QHBoxLayout();
    cardsRow->setSpacing(12);
    int cardCount = 0;
    for (auto it = data.begin(); it != data.end() && cardCount < 4; ++it) {
        if (it.value().isObject() || it.value().isArray()) continue;

        auto* card = new QWidget();
        QString bgColors[] = {"#4f46e5", "#0891b2", "#059669", "#d97706"};
        card->setStyleSheet(
            QString("QWidget { background: %1; border-radius: 8px; padding: 12px; }")
                .arg(bgColors[cardCount % 4])
        );
        auto* cl = new QVBoxLayout(card);
        cl->setContentsMargins(8, 6, 8, 6);
        cl->setSpacing(2);

        auto* valLabel = new QLabel(it.value().toVariant().toString());
        valLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
        cl->addWidget(valLabel);

        auto* nameLabel = new QLabel(it.key().replace("_", " "));
        nameLabel->setStyleSheet("font-size: 10px; color: rgba(255,255,255,0.8);");
        cl->addWidget(nameLabel);

        cardsRow->addWidget(card);
        cardCount++;
    }
    layout->addLayout(cardsRow);

    // Detailed table
    auto* table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Metric", "Value"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
        "QTableWidget::item:alternate { background: palette(alternate-base); }"
    );

    // Flatten nested JSON for table display
    QList<QPair<QString, QString>> flatRows;
    std::function<void(const QJsonObject&, const QString&)> flatten;
    flatten = [&flatten, &flatRows](const QJsonObject& obj, const QString& prefix) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QString key = prefix.isEmpty() ? it.key() : prefix + " > " + it.key();
            if (it.value().isObject()) {
                flatten(it.value().toObject(), key);
            } else if (it.value().isArray()) {
                flatRows.append({key, QString("[%1 items]").arg(it.value().toArray().size())});
            } else {
                flatRows.append({key, it.value().toVariant().toString()});
            }
        }
    };
    flatten(data, "");

    table->setRowCount(flatRows.size());
    for (int i = 0; i < flatRows.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(flatRows[i].first));
        table->setItem(i, 1, new QTableWidgetItem(flatRows[i].second));
    }
    layout->addWidget(table, 1);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::showAiStatusDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("AI Service Status");
    dlg.setMinimumSize(500, 400);
    auto* layout = new QVBoxLayout(&dlg);

    // Status indicator
    bool available = data["available"].toBool(data["status"].toString() != "offline");
    QString statusColor = available ? "#059669" : "#dc2626";
    QString statusText = available ? "Online" : "Offline";

    auto* statusRow = new QHBoxLayout();
    auto* dot = new QLabel();
    dot->setFixedSize(16, 16);
    dot->setStyleSheet(QString("background: %1; border-radius: 8px;").arg(statusColor));
    statusRow->addWidget(dot);
    auto* statusLabel = new QLabel("AI Service: " + statusText);
    statusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(statusColor));
    statusRow->addWidget(statusLabel);
    statusRow->addStretch();
    layout->addLayout(statusRow);

    // Details table
    auto* table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Property", "Value"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    QList<QPair<QString, QString>> rows;
    for (auto it = data.begin(); it != data.end(); ++it) {
        if (it.value().isObject() || it.value().isArray()) continue;
        rows.append({it.key().replace("_", " "), it.value().toVariant().toString()});
    }
    table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(rows[i].first));
        table->setItem(i, 1, new QTableWidgetItem(rows[i].second));
    }
    layout->addWidget(table, 1);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);

    dlg.exec();
}

void MainWindow::showCrawlerWorkersDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("Crawler Workers");
    dlg.setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray workers = data["workers"].toArray(data["data"].toArray());
    auto* table = new QTableWidget(workers.size(), 5);
    table->setHorizontalHeaderLabels({"ID", "Status", "Task", "Uptime", "Papers Crawled"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < workers.size(); ++i) {
        auto w = workers[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(w["id"].toInt())));
        QString status = w["status"].toString("unknown");
        auto* statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(status == "active" ? QBrush(QColor("#059669")) : QBrush(QColor("#6b7280")));
        table->setItem(i, 1, statusItem);
        table->setItem(i, 2, new QTableWidgetItem(QString::number(w["task_id"].toInt(w["current_task"].toInt()))));
        table->setItem(i, 3, new QTableWidgetItem(w["uptime"].toString(w["running_since"].toString("--")));
        table->setItem(i, 4, new QTableWidgetItem(QString::number(w["papers_crawled"].toInt(w["items_processed"].toInt(0)))));
    }
    layout->addWidget(table, 1);

    // Summary
    int activeCount = 0;
    for (const auto& w : workers) {
        if (w.toObject()["status"].toString() == "active") activeCount++;
    }
    auto* summary = new QLabel(QString("Total: %1 workers | Active: %2").arg(workers.size()).arg(activeCount));
    summary->setStyleSheet("color: palette(mid); font-size: 12px; padding: 8px;");
    layout->addWidget(summary);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::showCrawlerStatisticsDialog(const QJsonObject& data) {
    showStatsDialog("Crawler Statistics", data);
}

void MainWindow::showSearchHistoryDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("Search History");
    dlg.setMinimumSize(600, 400);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray history = data["history"].toArray(data["data"].toArray(data["searches"].toArray()));
    auto* table = new QTableWidget(history.size(), 3);
    table->setHorizontalHeaderLabels({"Query", "Results", "Time"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < history.size(); ++i) {
        auto h = history[i].toObject();
        QString query = h["query"].toString(h["keyword"].toString());
        auto* queryItem = new QTableWidgetItem(query);
        queryItem->setForeground(QBrush(QColor("#4f46e5")));
        queryItem->setCursor(Qt::PointingHandCursor);
        table->setItem(i, 0, queryItem);
        table->setItem(i, 1, new QTableWidgetItem(QString::number(h["result_count"].toInt(h["total"].toInt(0)))));
        table->setItem(i, 2, new QTableWidgetItem(h["created_at"].toString(h["timestamp"].toString(h["time"].toString()))));
    }

    connect(table, &QTableWidget::cellDoubleClicked, this, [this, table, history](int row, int) {
        if (row >= 0 && row < history.size()) {
            QString query = history[row].toObject()["query"].toString(history[row].toObject()["keyword"].toString());
            if (!query.isEmpty()) {
                onSearch(query);
                searchWidget_->setText(query);
            }
        }
    });

    layout->addWidget(table, 1);

    auto* hint = new QLabel("Double-click a row to search again");
    hint->setStyleSheet("color: palette(mid); font-size: 11px; padding: 4px;");
    layout->addWidget(hint);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::showAllPapersDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("All Papers");
    dlg.setMinimumSize(800, 550);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray papers = data["papers"].toArray(data["data"].toArray());
    int total = data["total"].toInt(papers.size());

    auto* infoLabel = new QLabel(QString("Showing %1 of %2 papers").arg(papers.size()).arg(total));
    infoLabel->setStyleSheet("color: palette(mid); font-size: 12px; font-weight: bold;");
    layout->addWidget(infoLabel);

    auto* table = new QTableWidget(papers.size(), 5);
    table->setHorizontalHeaderLabels({"ID", "Title", "Journal", "Year", "Level"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); "
        "font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < papers.size(); ++i) {
        auto p = papers[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(QString::number(p["id"].toInt())));
        table->setItem(i, 1, new QTableWidgetItem(p["title"].toString().left(80)));
        table->setItem(i, 2, new QTableWidgetItem(p["journal"].toString(p["journal_name"].toString())));
        table->setItem(i, 3, new QTableWidgetItem(p["year"].toString()));
        table->setItem(i, 4, new QTableWidgetItem(p["level"].toString()));
    }

    connect(table, &QTableWidget::cellDoubleClicked, this, [this, papers](int row, int) {
        if (row >= 0 && row < papers.size()) {
            int pid = papers[row].toObject()["id"].toInt();
            onPaperSelected(pid);
        }
    });

    layout->addWidget(table, 1);

    auto* hint = new QLabel("Double-click to view paper details");
    hint->setStyleSheet("color: palette(mid); font-size: 11px; padding: 4px;");
    layout->addWidget(hint);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::showRecommendationExplanationDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("Why this recommendation?");
    dlg.setMinimumSize(500, 350);
    auto* layout = new QVBoxLayout(&dlg);

    QString title = data["title"].toString(data["paper_title"].toString("Paper"));
    auto* titleLabel = new QLabel("Why we recommend: " + title.left(60));
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
    layout->addWidget(titleLabel);

    auto* divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: palette(mid);");
    layout->addWidget(divider);

    // Explanation
    QString explanation = data["explanation"].toString(data["reason"].toString(data["text"].toString())));
    if (!explanation.isEmpty()) {
        auto* expLabel = new QLabel(explanation);
        expLabel->setWordWrap(true);
        expLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        expLabel->setStyleSheet("color: palette(text); font-size: 14px; line-height: 1.5; padding: 8px;");
        layout->addWidget(expLabel, 1);
    }

    // Score
    QString score = data["score"].toString(data["relevance_score"].toString());
    if (!score.isEmpty()) {
        auto* scoreLabel = new QLabel("Relevance Score: " + score.left(6));
        scoreLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #4f46e5; padding: 8px;");
        layout->addWidget(scoreLabel);
    }

    // Similar factors
    QJsonArray factors = data["factors"].toArray(data["reasons"].toArray());
    if (!factors.isEmpty()) {
        auto* factorsLabel = new QLabel("Key Factors:");
        factorsLabel->setStyleSheet("font-weight: bold; color: palette(text); font-size: 12px; margin-top: 8px;");
        layout->addWidget(factorsLabel);
        for (const auto& f : factors) {
            QString factorText = f.isObject() ? f.toObject()["name"].toString(f.toObject()["text"].toString()) : f.toString();
            if (!factorText.isEmpty()) {
                auto* fLabel = new QLabel("  - " + factorText);
                fLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
                layout->addWidget(fLabel);
            }
        }
    }

    layout->addStretch();

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
}

void MainWindow::showLatexTemplatesDialog(const QJsonObject& data) {
    QDialog dlg(this);
    dlg.setWindowTitle("LaTeX Templates");
    dlg.setMinimumSize(600, 450);
    auto* layout = new QVBoxLayout(&dlg);

    QJsonArray templates = data["templates"].toArray(data["data"].toArray());
    if (templates.isEmpty()) {
        auto* emptyLabel = new QLabel("No templates available.\nCreate documents from scratch or add templates on the server.");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: palette(mid); font-size: 14px; padding: 40px;");
        layout->addWidget(emptyLabel);
        auto* closeBtn = new QPushButton("Close");
        closeBtn->setStyleSheet("QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); border-radius: 6px; padding: 8px 24px; font-weight: bold; }");
        connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
        layout->addWidget(closeBtn, 0, Qt::AlignRight);
        dlg.exec();
        return;
    }

    auto* table = new QTableWidget(templates.size(), 3);
    table->setHorizontalHeaderLabels({"Name", "Category", "Action"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(
        "QTableWidget { background: palette(base); border: 1px solid palette(mid); border-radius: 8px; }"
        "QHeaderView::section { background: palette(window); color: palette(text); font-weight: bold; padding: 6px; border: none; }"
        "QTableWidget::item { padding: 4px; color: palette(text); }"
    );

    for (int i = 0; i < templates.size(); ++i) {
        auto t = templates[i].toObject();
        table->setItem(i, 0, new QTableWidgetItem(t["name"].toString()));
        table->setItem(i, 1, new QTableWidgetItem(t["category"].toString()));

        auto* applyBtn = new QPushButton("Apply");
        applyBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 11px; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        QString content = t["content"].toString(t["template"].toString());
        connect(applyBtn, &QPushButton::clicked, this, [this, &dlg, content]() {
            if (latexEditor_) {
                latexEditor_->loadContent(content);
            }
            dlg.accept();
            statusBar()->showMessage("Template applied", 3000);
        });
        table->setCellWidget(i, 2, applyBtn);
    }
    layout->addWidget(table, 1);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
    );
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
    dlg.exec();
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

    editMenu->addSeparator();

    auto* advSearchAction = editMenu->addAction("&Advanced Search");
    advSearchAction->setShortcut(QKeySequence("Ctrl+Shift+F"));
    connect(advSearchAction, &QAction::triggered, this, [this]() {
        auto* dlg = new AdvancedSearchDialog(this);
        connect(dlg, &AdvancedSearchDialog::searchRequested, this, [this](const AdvancedSearchDialog::SearchCriteria& c) {
            QString query = c.query;
            if (!c.title.isEmpty()) query += QString(" title:\"%1\"").arg(c.title);
            if (!c.author.isEmpty()) query += QString(" author:\"%1\"").arg(c.author);
            if (!c.venue.isEmpty()) query += QString(" venue:\"%1\"").arg(c.venue);
            if (!c.yearFrom.isEmpty()) query += QString(" year:>=%1").arg(c.yearFrom);
            if (!c.yearTo.isEmpty()) query += QString(" year:<=%1").arg(c.yearTo);
            if (!c.level.isEmpty()) query += QString(" level:%1").arg(c.level);
            onSearch(query.trimmed());
            tabWidget_->setCurrentIndex(0);
            ToastWidget::showInfo("Searching...");
        });
        dlg->exec();
        dlg->deleteLater();
    });

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

    toolsMenu->addSeparator();

    QAction* crawlerAction = toolsMenu->addAction("&Crawler Dashboard");
    connect(crawlerAction, &QAction::triggered, this, [this]() {
        tabWidget_->setCurrentIndex(2);
        apiManager_->getCrawlerDashboard();
    });

    QAction* aiAction = toolsMenu->addAction("&AI Assistant");
    connect(aiAction, &QAction::triggered, this, [this]() {
        tabWidget_->setCurrentIndex(3);
    });

    toolsMenu->addSeparator();

    auto* readingListAction = toolsMenu->addAction("&Reading Lists");
    connect(readingListAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Reading Lists");
        dlg->resize(700, 450);
        auto* layout = new QVBoxLayout(dlg);
        auto* mgr = new ReadingListManager(apiManager_);
        layout->addWidget(mgr);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* cacheAction = toolsMenu->addAction("&Offline Cache");
    connect(cacheAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Offline Cache Manager");
        dlg->resize(800, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* mgr = new OfflineCacheManager(localDb_);
        mgr->refreshPapers();
        layout->addWidget(mgr);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* historyAction = toolsMenu->addAction("Recent &History");
    historyAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(historyAction, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Recently Viewed Papers");
        dlg->resize(400, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* hist = new RecentHistoryWidget();
        connect(hist, &RecentHistoryWidget::openPaperRequested, this, [this](int paperId) {
            apiManager_->getPaperDetails(paperId);
        });
        layout->addWidget(hist);
        dlg->exec();
        dlg->deleteLater();
    });

    toolsMenu->addSeparator();

    auto* shortcutAction = toolsMenu->addAction("Keyboard &Shortcuts");
    connect(shortcutAction, &QAction::triggered, this, [this]() {
        auto* dlg = new ShortcutConfigDialog(this);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* timelineAction = toolsMenu->addAction("Paper &Timeline");
    connect(timelineAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.isEmpty()) {
            ToastWidget::showWarning("No papers to display. Search first.");
            return;
        }
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Paper Timeline");
        dlg->resize(900, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* timeline = new PaperTimelineWidget();
        timeline->setPapers(papers);
        connect(timeline, &PaperTimelineWidget::paperClicked, this, [this](int paperId) {
            apiManager_->getPaperDetails(paperId);
        });
        layout->addWidget(timeline);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* graphAction = toolsMenu->addAction("Citation &Graph");
    connect(graphAction, &QAction::triggered, this, [this]() {
        if (!resultView_) return;
        auto papers = resultView_->getPapers();
        if (papers.isEmpty()) {
            ToastWidget::showWarning("No papers to display. Search first.");
            return;
        }
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("Citation Graph");
        dlg->resize(900, 600);
        auto* layout = new QVBoxLayout(dlg);
        auto* graph = new CitationGraphWidget();
        graph->setPapers(papers);
        connect(graph, &CitationGraphWidget::paperClicked, this, [this, graph](int paperId) {
            apiManager_->getPaperDetails(paperId);
            graph->setFocusPaper(paperId);
        });
        layout->addWidget(graph);
        dlg->exec();
        dlg->deleteLater();
    });

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
        "  background-color: palette(window);"
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

    QAction* settingsAction = toolBar->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onPreferences);

    toolBar->addSeparator();

    // Auth actions
    loginAction_ = toolBar->addAction("Login");
    connect(loginAction_, &QAction::triggered, this, &MainWindow::showLoginDialog);

    logoutAction_ = toolBar->addAction("Logout");
    logoutAction_->setVisible(false);
    connect(logoutAction_, &QAction::triggered, this, &MainWindow::handleLogout);

    profileAction_ = toolBar->addAction("Profile");
    profileAction_->setVisible(false);
    connect(profileAction_, &QAction::triggered, this, [this]() {
        if (!authManager_ || !authManager_->isAuthenticated()) return;
        DesktopUser user = authManager_->getCurrentUser();
        QString info = QString("Username: %1\nEmail: %2\nRole: %3")
            .arg(user.username, user.email, user.role);
        QMessageBox::information(this, "Profile", info);
    });

    toolBar->addSeparator();

    auto* favoritesAction = toolBar->addAction("Favorites");
    connect(favoritesAction, &QAction::triggered, this, [this]() {
        tabWidget_->setCurrentIndex(1);
    });

    // User label in status bar
    userLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(userLabel_);
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

    // Recent history
    recentHistory_ = new RecentHistoryWidget(this);

    // Batch operations bar
    batchBar_ = new BatchOperationsBar(this);
    connect(batchBar_, &BatchOperationsBar::batchFavorite, this, [this](const QList<int>& ids, bool fav) {
        for (int id : ids) apiManager_->togglePaperFavorite(id, fav);
        ToastWidget::showSuccess(QString("%1 %2 papers").arg(fav ? "Favorited" : "Unfavorited").arg(ids.size()));
        batchBar_->clearSelection();
    });
    connect(batchBar_, &BatchOperationsBar::batchExport, this, [this](const QList<int>& ids) {
        Q_UNUSED(ids);
        onExport();
        batchBar_->clearSelection();
    });
    connect(batchBar_, &BatchOperationsBar::batchDelete, this, [this](const QList<int>& ids) {
        for (int id : ids) apiManager_->deletePaper(id);
        ToastWidget::showSuccess(QString("Deleted %1 papers").arg(ids.size()));
        batchBar_->clearSelection();
        if (!currentKeyword_.isEmpty()) {
            apiManager_->searchPapers(currentKeyword_, "", "", currentOffset_, currentLimit_);
        }
    });
    connect(batchBar_, &BatchOperationsBar::batchAddTags, this, [this](const QList<int>& ids, const QStringList& tags) {
        for (int id : ids) apiManager_->addPaperTags(id, tags);
        ToastWidget::showSuccess(QString("Added tags to %1 papers").arg(ids.size()));
        batchBar_->clearSelection();
    });

    // System tray
    trayManager_ = new SystemTrayManager(this);
    connect(trayManager_, &SystemTrayManager::showWindowRequested, this, [this]() {
        showNormal();
        activateWindow();
    });
    connect(trayManager_, &SystemTrayManager::searchRequested, this, [this](const QString& q) {
        showNormal();
        activateWindow();
        onSearch(q);
    });
    connect(trayManager_, &SystemTrayManager::quitRequested, this, []() {
        QApplication::quit();
    });

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
    connect(apiManager_, &ApiManager::paperDetailsSuccess,
            this, &MainWindow::onPaperDetailsSuccess);
    connect(apiManager_, &ApiManager::networkError,
            this, &MainWindow::onNetworkError);
    connect(apiManager_, &ApiManager::recentPapersSuccess,
            this, [this](const QList<Paper>& papers) {
        auto* grid = findChild<QWidget*>("recentPapersGrid");
        if (!grid) return;
        auto* gridLayout = grid->findChild<QHBoxLayout*>("recentGridLayout");
        if (!gridLayout) return;

        // Clear old cards
        QLayoutItem* item;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        for (const auto& paper : papers) {
            auto* card = new QWidget();
            card->setStyleSheet(
                "QWidget { background: palette(base); border: 1px solid palette(mid); "
                "border-radius: 8px; }"
            );
            card->setCursor(Qt::PointingHandCursor);
            card->setMaximumWidth(280);
            card->setMinimumHeight(100);

            auto* layout = new QVBoxLayout(card);
            layout->setContentsMargins(12, 10, 12, 10);
            layout->setSpacing(4);

            auto* titleLabel = new QLabel(paper.title.left(80));
            titleLabel->setWordWrap(true);
            titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: palette(text);");
            layout->addWidget(titleLabel);

            auto* meta = new QLabel(
                (paper.journalShort.isEmpty() ? paper.journalFull : paper.journalShort)
                + " | " + paper.year
            );
            meta->setStyleSheet("font-size: 10px; color: palette(mid);");
            layout->addWidget(meta);

            int pid = paper.id;
            connect(card, &QWidget::mousePressEvent, this, [this, pid](QMouseEvent*) {
                onPaperSelected(pid);
            });

            gridLayout->addWidget(card);
        }
    });

    // Handle API failure signals
    connect(apiManager_, &ApiManager::healthCheckFailed, this, [this](const QString& error) {
        auto* healthLabel = findChild<QLabel*>("healthIndicator");
        if (!healthLabel) {
            healthLabel = new QLabel(this);
            healthLabel->setObjectName("healthIndicator");
            statusBar()->addPermanentWidget(healthLabel);
        }
        healthLabel->setStyleSheet(
            "padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; "
            "color: #dc2626; background: #fee2e2;"
        );
        healthLabel->setText("Offline");
        setWindowTitle("PaperCrawler - Offline");
    });

    connect(apiManager_, &ApiManager::paperDetailsFailed, this, [this](const QString& error) {
        statusBar()->showMessage("Failed to load paper details: " + error.left(50), 5000);
    });

    connect(apiManager_, &ApiManager::statsFailed, this, [this](const QString& error) {
        auto* statsContent = findChild<QLabel*>("statsContent");
        if (statsContent) {
            statsContent->setText("<p style='color: #dc2626;'>Failed to load statistics. Server may be offline.</p>");
        }
    });

    connect(apiManager_, &ApiManager::apiError, this, [this](const QString& error) {
        statusBar()->showMessage("API error: " + error.left(50), 5000);
    });

    // Filter panel
    if (filterPanel_) {
        connect(filterPanel_, &FilterPanel::filterChanged,
                this, [this](const QString& level, const QString& year, const QString& type) {
            Q_UNUSED(type);
            if (!currentKeyword_.isEmpty()) {
                currentOffset_ = 0;
                apiManager_->searchPapers(currentKeyword_, year, level, 0, currentLimit_);
            }
        });
    }

    // Tab switch data loading
    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int index) {
        switch (index) {
            case 1: // Favorites
                refreshFavoritesTab();
                break;
            case 2: // Crawler - auto-refresh running tasks
                apiManager_->getCrawlerDashboard();
                apiManager_->getCrawlerTasks();
                apiManager_->getCrawlerTemplates();
                apiManager_->getCrawlerSchedules();
                break;
            case 4: // Statistics
                apiManager_->getStats("overview");
                break;
            case 5: // Recommendations
                apiManager_->getPaperRecommendations();
                break;
            case 6: // Admin
                apiManager_->getAdminDashboard();
                apiManager_->getAdminUsers();
                apiManager_->getAdminModules();
                apiManager_->getSystemMonitor();
                break;
            case 7: // LaTeX Editor
                apiManager_->listLatexDocuments();
                apiManager_->listLatexTemplates();
                break;
        }
    });

    // Crawler auto-refresh timer (poll every 15s when on crawler tab)
    auto* crawlerRefreshTimer = new QTimer(this);
    connect(crawlerRefreshTimer, &QTimer::timeout, this, [this]() {
        if (tabWidget_->currentIndex() == 2) {
            apiManager_->getCrawlerDashboard();
            apiManager_->getCrawlerTasks();
        }
    });
    crawlerRefreshTimer->start(15000);

    // === Recommendations: generic JSON responses ===
    connect(apiManager_, &ApiManager::jsonResponse, this,
        [this](const QString& endpoint, const QJsonObject& data) {
            if (endpoint.contains("recommendations")) {
                populateRecommendations(data);
            } else if (endpoint.contains("trending")) {
                QJsonArray papers = data["papers"].toArray(data["data"].toArray());
                QJsonObject wrapped;
                wrapped["papers"] = papers;
                wrapped["total"] = papers.size();
                wrapped["source"] = "trending";
                populateRecommendations(wrapped);
            } else if (endpoint.contains("similar")) {
                QJsonArray papers = data["papers"].toArray(data["similar"].toArray());
                QJsonObject wrapped;
                wrapped["papers"] = papers;
                wrapped["total"] = papers.size();
                wrapped["source"] = "similar";
                populateRecommendations(wrapped);
            } else if (endpoint.contains("admin/dashboard")) {
                populateAdminDashboard(data);
            } else if (endpoint.contains("admin/users")) {
                populateAdminUsers(data);
            } else if (endpoint.contains("admin/modules") || endpoint.contains("modules")) {
                populateAdminModules(data);
            } else if (endpoint.contains("monitor")) {
                populateAdminMonitor(data);
            } else if (endpoint.contains("performance")) {
                populatePerformanceMetrics(data);
            } else if (endpoint.contains("login")) {
                showLoginHistory(data);
            } else if (endpoint.contains("recommendation") && endpoint.contains("feedback")) {
                statusBar()->showMessage("Feedback submitted. Thank you!", 3000);
            } else if (endpoint.contains("recommendation") && endpoint.contains("explanation")) {
                showRecommendationExplanationDialog(data);
            } else if (endpoint.contains("ai/summarize")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>AI Summary:</b> " + data["summary"].toString(data["text"].toString("Summary completed.")));
            } else if (endpoint.contains("ai/compare")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>AI Compare:</b> " + data["comparison"].toString(data["text"].toString("Comparison completed.")));
            } else if (endpoint.contains("ai/keywords")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                QString kws = data["keywords"].toString(data["text"].toString());
                if (kws.isEmpty() && data.contains("keywords")) {
                    QJsonArray kwArr = data["keywords"].toArray();
                    QStringList kwList;
                    for (const auto& kw : kwArr) kwList << kw.toString();
                    kws = kwList.join(", ");
                }
                if (aiChat) aiChat->append("<b>AI Keywords:</b> " + (kws.isEmpty() ? "Keywords extraction completed." : kws));
            } else if (endpoint.contains("ai/contributions")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>AI Contributions:</b> " + data["contributions"].toString(data["text"].toString("Contributions analysis completed.")));
            } else if (endpoint.contains("ai/copilot/review")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>Copilot Review:</b> " + data["review"].toString(data["text"].toString("Copilot review completed.")));
            } else if (endpoint.contains("ai/literature")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                QString review = data["review"].toString(data["literature_review"].toString(data["text"].toString()));
                if (aiChat) aiChat->append("<b>Literature Review:</b> " + (review.isEmpty() ? "Generated successfully." : review));
            } else if (endpoint.contains("ai/research")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                QString plan = data["plan"].toString(data["research_plan"].toString(data["text"].toString()));
                if (aiChat) aiChat->append("<b>Research Plan:</b> " + (plan.isEmpty() ? "Generated successfully." : plan));
            } else if (endpoint.contains("ai/status")) {
                showAiStatusDialog(data);
            } else if (endpoint.contains("ai/copilot/recommendations")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>Copilot Recommendations:</b> " + data["recommendations"].toString(data["text"].toString("Recommendations loaded.")));
            } else if (endpoint.contains("ai/copilot/stats")) {
                auto* aiChat = findChild<QTextEdit*>("aiChatDisplay");
                if (aiChat) aiChat->append("<b>Copilot Stats:</b> Reviews: " +
                    QString::number(data["total_reviews"].toInt(0)) +
                    " | Plans: " + QString::number(data["total_plans"].toInt(0)));
            } else if (endpoint.contains("crawler/schedules")) {
                populateCrawlerSchedules(data);
            } else if (endpoint.contains("crawler/workers")) {
                showCrawlerWorkersDialog(data);
            } else if (endpoint.contains("crawler/statistics")) {
                showCrawlerStatisticsDialog(data);
            } else if (endpoint.contains("crawler/templates")) {
                populateCrawlerTemplates(data);
            } else if (endpoint.contains("export/formats")) {
                statusBar()->showMessage("Export formats loaded", 2000);
            } else if (endpoint.contains("search/saved")) {
                populateSavedSearches(data);
            } else if (endpoint.contains("search/trending")) {
                populateTrendingSearches(data);
            } else if (endpoint.contains("search/history")) {
                showSearchHistoryDialog(data);
            } else if (endpoint.contains("papers") && endpoint.contains("all")) {
                showAllPapersDialog(data);
            } else if (endpoint.contains("stats/system")) {
                showStatsDialog("System Statistics", data);
            } else if (endpoint.contains("stats/resource")) {
                showStatsDialog("Resource Statistics", data);
            } else if (endpoint.contains("stats/performance")) {
                showStatsDialog("Performance Statistics", data);
            } else if (endpoint.contains("latex/documents") && endpoint.contains("create")) {
                // New document created
                int docId = data["id"].toInt(0);
                if (docId > 0 && latexEditor_) {
                    statusBar()->showMessage(QString("Document created (ID: %1)").arg(docId), 5000);
                    apiManager_->listLatexDocuments();
                }
            } else if (endpoint.contains("latex/documents/update")) {
                statusBar()->showMessage("Document saved", 2000);
            } else if (endpoint.contains("latex/documents/delete")) {
                statusBar()->showMessage("Document deleted", 3000);
                apiManager_->listLatexDocuments();
            } else if (endpoint.contains("latex/compile")) {
                if (latexEditor_) {
                    bool ok = data["success"].toBool();
                    QString pdf = data["pdfPath"].toString(data["pdf_path"].toString());
                    QString err = data["error"].toString(data["errorMessage"].toString());
                    auto* preview = latexEditor_->findChild<LatexPreviewWidget*>();
                    if (preview) preview->showCompileResult(ok, pdf, err);
                }
                statusBar()->showMessage(data["success"].toBool() ? "Compilation successful" : "Compilation failed", 5000);
            } else if (endpoint.contains("latex/autosave")) {
                // Silent auto-save
            } else if (endpoint.contains("latex/templates")) {
                showLatexTemplatesDialog(data);
            } else if (endpoint.contains("latex/document") && !endpoint.contains("create") && !endpoint.contains("update") && !endpoint.contains("delete")) {
                // Single document loaded
                if (latexEditor_) {
                    latexEditor_->loadContent(data["content"].toString());
                }
            }
        });

    connect(apiManager_, &ApiManager::jsonArrayResponse, this,
        [this](const QString& endpoint, const QJsonArray& data) {
            if (endpoint.contains("recommendations")) {
                QJsonObject wrapped;
                wrapped["papers"] = data;
                wrapped["total"] = data.size();
                wrapped["source"] = "recommendations";
                populateRecommendations(wrapped);
            } else if (endpoint.contains("trending")) {
                QJsonObject wrapped;
                wrapped["papers"] = data;
                wrapped["total"] = data.size();
                wrapped["source"] = "trending";
                populateRecommendations(wrapped);
            } else if (endpoint.contains("admin/users")) {
                QJsonObject wrapped;
                wrapped["users"] = data;
                populateAdminUsers(wrapped);
            } else if (endpoint.contains("admin/modules") || endpoint.contains("modules")) {
                QJsonObject wrapped;
                wrapped["modules"] = data;
                populateAdminModules(wrapped);
            } else if (endpoint.contains("crawler/schedules")) {
                QJsonObject wrapped;
                wrapped["schedules"] = data;
                populateCrawlerSchedules(wrapped);
            } else if (endpoint.contains("crawler/templates")) {
                QJsonObject wrapped;
                wrapped["templates"] = data;
                populateCrawlerTemplates(wrapped);
            } else if (endpoint.contains("search/saved")) {
                QJsonObject wrapped;
                wrapped["searches"] = data;
                populateSavedSearches(wrapped);
            } else if (endpoint.contains("search/trending")) {
                QJsonObject wrapped;
                wrapped["trending"] = data;
                populateTrendingSearches(wrapped);
            } else if (endpoint.contains("latex/documents")) {
                // Document list (array) — populate combo
                if (latexEditor_) {
                    auto* combo = latexEditor_->findChild<QComboBox*>();
                    if (combo) {
                        combo->clear();
                        combo->addItem("New Document", 0);
                        for (const auto& val : data) {
                            auto d = val.toObject();
                            combo->addItem(d["title"].toString(d["name"].toString()), d["id"].toInt());
                        }
                    }
                }
            } else if (endpoint.contains("latex/templates")) {
                showLatexTemplatesDialog(data);
            }
        });

    connect(apiManager_, &ApiManager::genericError, this,
        [this](const QString& context, const QString& error) {
            statusBar()->showMessage(
                QString("%1 failed: %2").arg(context, error.left(80)), 5000);
        });
}

void MainWindow::loadSettings() {
    QSettings settings("PaperCrawler", "Desktop");

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // Apply saved theme
    int themeIndex = settings.value("display/theme", 0).toInt();
    if (themeIndex == 2 && themeManager_) {
        // Dark
        if (themeManager_->currentTheme() != ThemeManager::ThemeMode::Dark) {
            themeManager_->toggleTheme();
            themeButton_->setText("\xe2\x98\x80\xef\xb8\x8f");
        }
    } else if (themeIndex == 1 && themeManager_) {
        // Light
        if (themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark) {
            themeManager_->toggleTheme();
            themeButton_->setText("\xf0\x9f\x8c\x99");
        }
    }

    // Apply saved page size
    int savedPageSize = settings.value("display/pageSize", 20).toInt();
    currentLimit_ = savedPageSize;

    // Apply saved server URL
    QString savedUrl = settings.value("server/url", "http://localhost:8080").toString();
    if (apiManager_) {
        apiManager_->setBaseUrl(savedUrl);
        if (authManager_) {
            authManager_->setBaseUrl(savedUrl);
        }
    }
}

void MainWindow::saveSettings() {
    QSettings settings("PaperCrawler", "Desktop");

    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // Save theme preference
    if (themeManager_) {
        int themeIndex = themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark ? 2 : 1;
        settings.setValue("display/theme", themeIndex);
    }
    settings.setValue("display/pageSize", currentLimit_);
}

void MainWindow::onSearch(const QString& keyword) {
    if (keyword.isEmpty()) {
        QMessageBox::warning(this, "Search", "Please enter a search keyword.");
        return;
    }

    // Save search state for pagination
    currentKeyword_ = keyword;
    currentOffset_ = 0;
    currentLimit_ = 20;

    statusBar()->showMessage("Searching: " + keyword + "...");
    resultView_->setVisible(true);
    resultView_->clear();
    resultView_->setHighlightKeyword(keyword);
    filterPanel_->setVisible(true);

    // Hide hero section when searching
    if (heroWidget_) heroWidget_->setVisible(false);
    if (featureCards_) featureCards_->setVisible(false);

    // Show loading indicator in status bar
    auto* progressBar = findChild<QProgressBar*>("searchProgress");
    if (!progressBar) {
        progressBar = new QProgressBar(this);
        progressBar->setObjectName("searchProgress");
        progressBar->setRange(0, 0);  // Indeterminate
        progressBar->setMaximumWidth(200);
        progressBar->setMaximumHeight(16);
        progressBar->setTextVisible(false);
        progressBar->setStyleSheet(
            "QProgressBar { border: 1px solid palette(mid); border-radius: 4px; background: palette(base); }"
            "QProgressBar::chunk { background: #4f46e5; border-radius: 3px; }"
        );
        statusBar()->addPermanentWidget(progressBar);
    }
    progressBar->setVisible(true);

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
    // Hide loading indicator
    auto* progressBar = findChild<QProgressBar*>("searchProgress");
    if (progressBar) progressBar->setVisible(false);

    totalResults_ = result.total;

    qDebug() << "=== Backend Search Results ===";
    qDebug() << "Papers received:" << result.papers.size();
    qDebug() << "Total papers:" << result.total;
    qDebug() << "Offset:" << currentOffset_ << "Limit:" << currentLimit_;

    // Convert Paper results for display
    QList<Paper> papers;
    for (const auto& p : result.papers) {
        Paper paper;
        paper.id = p.id;
        paper.title = p.title;
        paper.journal = p.journalShort.isEmpty()
                       ? p.journalFull
                       : p.journalShort;
        paper.year = p.year;
        paper.level = p.level;
        paper.authors = p.authors;
        paper.doiUrl = p.doiUrl;
        papers.append(paper);
    }

    // Cache the results for this page
    paperCache_->insert(currentKeyword_, currentOffset_, currentLimit_, papers, totalResults_);

    // Save to local database
    if (localDb_ && localDb_->isOpen()) {
        for (const auto& paper : papers) {
            if (!localDb_->existsByTitle(paper.title)) {
                localDb_->savePaper(DbPaper::fromPaper(paper));
            }
        }
    }

    // Save to search history (first page only)
    if (currentOffset_ == 0) {
        auto* searchHistory = findChild<SearchHistory*>();
        if (searchHistory) {
            searchHistory->addSearch(currentKeyword_, result.total);
        }
    }

    // Display results with correct page number
    int pageNum = (currentOffset_ / currentLimit_) + 1;
    resultView_->setPapers(papers, totalResults_, pageNum);

    // Update status bar
    int totalPages = (totalResults_ + currentLimit_ - 1) / currentLimit_;
    QString message = QString("Page %1/%2 - Found %3 papers for \"%4\"")
                         .arg(pageNum).arg(totalPages).arg(result.total).arg(currentKeyword_);
    if (result.durationMs > 0) {
        message += QString(" (%1 ms)").arg(result.durationMs, 0, 'f', 2);
    }
    statusBar()->showMessage(message, 5000);

    // Update Search tab badge with result count
    if (tabWidget_) {
        tabWidget_->setTabText(0, QString("Search (%1)").arg(totalResults_));
    }

    // Log cache statistics
    qDebug() << "Cache statistics for" << currentKeyword_ << ":"
             << "Cached pages:" << paperCache_->getCacheCount(currentKeyword_)
             << "Total cache size:" << paperCache_->getCacheSize();

    ToastWidget::showSuccess(QString("Found %1 results for \"%2\"").arg(totalResults_).arg(currentKeyword_));
}

void MainWindow::onSearchFailed(const QString& error) {
    // Hide loading indicator
    auto* progressBar = findChild<QProgressBar*>("searchProgress");
    if (progressBar) progressBar->setVisible(false);

    // Try local database fallback
    if (localDb_ && localDb_->isOpen()) {
        DbSearchResult localResult = localDb_->searchPapers(currentKeyword_, currentOffset_, currentLimit_);
        if (!localResult.papers.isEmpty()) {
            resultView_->setPapers(localResult.papers, localResult.totalCount, 1);
            statusBar()->showMessage(
                QString("API offline — showing %1 local results for \"%2\"")
                    .arg(localResult.papers.size()).arg(currentKeyword_), 5000);
            return;
        }
    }

    resultView_->clear();
    QMessageBox::warning(this, "Search Failed",
        QString("Search error:\n%1\n\n"
                "Backend may be offline. Start it at http://localhost:8080").arg(error));
    statusBar()->showMessage("Search failed", 3000);
}

void MainWindow::onHealthCheckSuccess(bool healthy, const QString& message) {
    Q_UNUSED(message);
    qint64 latencyMs = healthTimer_.elapsed();

    if (heroWidget_) {
        heroWidget_->setHealthStatus(healthy
            ? HeroWidget::HealthStatus::Healthy
            : HeroWidget::HealthStatus::Unhealthy);
    }

    // Status bar health indicator
    auto* healthLabel = findChild<QLabel*>("healthIndicator");
    if (!healthLabel) {
        healthLabel = new QLabel(this);
        healthLabel->setObjectName("healthIndicator");
        statusBar()->addPermanentWidget(healthLabel);
    }
    healthLabel->setStyleSheet(
        QString("padding: 2px 8px; border-radius: 4px; font-size: 11px; font-weight: bold; "
                "color: %1; background: %2;")
        .arg(healthy ? "#059669" : "#dc2626", healthy ? "#d1fae5" : "#fee2e2")
    );
    healthLabel->setText(healthy ? QString("Online %1ms").arg(latencyMs) : "Offline");

    // Cache stats in status bar
    auto* cacheLabel = findChild<QLabel*>("cacheStats");
    if (!cacheLabel && paperCache_) {
        cacheLabel = new QLabel(this);
        cacheLabel->setObjectName("cacheStats");
        cacheLabel->setStyleSheet("color: palette(mid); font-size: 11px; padding: 2px 6px;");
        statusBar()->addPermanentWidget(cacheLabel);
    }
    if (cacheLabel && paperCache_) {
        cacheLabel->setText(QString("Cache: %1").arg(paperCache_->getCacheSize()));
    }

    // Window title with status
    QString title = healthy ? "PaperCrawler - Connected" : "PaperCrawler - Offline";
    if (authManager_ && authManager_->isAuthenticated()) {
        title += QString(" (%1)").arg(authManager_->getCurrentUser().username);
    }
    setWindowTitle(title);

    // Load recent papers on first health check success
    if (healthy) {
        apiManager_->getRecentPapers(6);
    }
}

void MainWindow::onNetworkError(const QString& error) {
    qWarning() << "Network error:" << error;
    statusBar()->showMessage("网络错误: " + error.left(50) + "...", 5000);
}

void MainWindow::onExport(ExportFormat format) {
    // Export dialog with format selection + source options
    QDialog dlg(this);
    dlg.setWindowTitle("Export Papers");
    dlg.setMinimumWidth(420);
    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    // Format selection
    auto* fmtLabel = new QLabel("Export Format:");
    fmtLabel->setStyleSheet("font-weight: bold; color: palette(text);");
    layout->addWidget(fmtLabel);

    auto* fmtCombo = new QComboBox();
    fmtCombo->addItems({"CSV", "BibTeX", "JSON", "EndNote", "RIS"});
    fmtCombo->setCurrentIndex(static_cast<int>(format));
    fmtCombo->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
    layout->addWidget(fmtCombo);

    // Source selection
    auto* srcLabel = new QLabel("Export Source:");
    srcLabel->setStyleSheet("font-weight: bold; color: palette(text);");
    layout->addWidget(srcLabel);

    auto* srcCombo = new QComboBox();
    srcCombo->addItems({"Current Search Results", "All Papers", "Favorites", "Recent Papers"});
    srcCombo->setStyleSheet("padding: 8px; border: 1px solid palette(mid); border-radius: 6px;");
    layout->addWidget(srcCombo);

    // Paper count info
    auto* countLabel = new QLabel("");
    countLabel->setStyleSheet("color: palette(mid); font-size: 11px;");
    int resultCount = resultView_ ? resultView_->paperCount() : 0;
    countLabel->setText(QString("Current results: %1 papers").arg(resultCount));
    layout->addWidget(countLabel);

    // Options
    auto* includeAbstract = new QCheckBox("Include Abstracts");
    includeAbstract->setChecked(true);
    layout->addWidget(includeAbstract);

    auto* includeTags = new QCheckBox("Include Tags");
    includeTags->setChecked(true);
    layout->addWidget(includeTags);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); "
        "border: 1px solid palette(mid); border-radius: 6px; padding: 8px 20px; }"
    );
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);

    auto* exportBtn = new QPushButton("Export");
    exportBtn->setStyleSheet(
        "QPushButton { background: #4f46e5; color: white; border: none; "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
        "QPushButton:hover { background: #4338ca; }"
    );
    btnRow->addWidget(exportBtn);
    layout->addLayout(btnRow);

    connect(exportBtn, &QPushButton::clicked, this, [this, &dlg, fmtCombo, srcCombo,
                                                       includeAbstract, includeTags, resultCount]() {
        QString fmtStr = fmtCombo->currentText().toLower();
        int srcIdx = srcCombo->currentIndex();

        // Try server-side export first
        QJsonObject params;
        params["format"] = fmtStr;
        params["include_abstract"] = includeAbstract->isChecked();
        params["include_tags"] = includeTags->isChecked();

        if (srcIdx == 0 && resultCount > 0) {
            // Current search results
            QList<Paper> papers = resultView_->getPapers();
            QJsonArray paperIds;
            for (const auto& p : papers) paperIds.append(p.id);
            params["paper_ids"] = paperIds;
            params["query"] = currentKeyword_;
        } else if (srcIdx == 1) {
            params["source"] = "all";
        } else if (srcIdx == 2) {
            params["source"] = "favorites";
        } else if (srcIdx == 3) {
            params["source"] = "recent";
            params["limit"] = 50;
        }

        apiManager_->exportData(params);
        dlg.accept();
        statusBar()->showMessage("Export requested...", 3000);

        // Also do local export as fallback
        if (srcIdx == 0 && resultCount > 0) {
            ExportFormat fmt = static_cast<ExportFormat>(fmtCombo->currentIndex());
            if (fmt < ExportFormat::PDF) {
                QString fileName = exportManager_->showSaveDialog(this, fmt);
                if (!fileName.isEmpty()) {
                    QList<Paper> papers = resultView_->getPapers();
                    bool ok = false;
                    switch (fmt) {
                        case ExportFormat::CSV: ok = exportManager_->exportToCSV(fileName, papers); break;
                        case ExportFormat::BibTeX: ok = exportManager_->exportToBibTeX(fileName, papers); break;
                        case ExportFormat::JSON: ok = exportManager_->exportToJSON(fileName, papers); break;
                        case ExportFormat::PDF: ok = exportManager_->exportToPDF(fileName, papers); break;
                    }
                    if (ok) statusBar()->showMessage(
                        QString("Exported %1 papers to %2").arg(papers.size()).arg(fileName), 5000);
                }
            }
        }
    });

    dlg.exec();
}

void MainWindow::onPreferences() {
    auto* dialog = new SettingsDialog(apiManager_, authManager_, this);
    dialog->exec();
    dialog->deleteLater();
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

        // Update LaTeX editor dark mode
        if (latexEditor_) {
            latexEditor_->setDarkMode(themeManager_->currentTheme() == ThemeManager::ThemeMode::Dark);
        }
    }
}

void MainWindow::onShowStatistics() {
    tabWidget_->setCurrentIndex(4);
    apiManager_->getStats("overview");
}

void MainWindow::onAbout() {
    int favCount = 0;
    auto* favMgr = findChild<FavoriteManager*>();
    if (favMgr) favCount = favMgr->getFavoriteCount();
    int cacheSize = paperCache_ ? paperCache_->getCacheSize() : 0;
    int dbCount = localDb_ ? localDb_->getPaperCount() : 0;

    QMessageBox::about(this, "About PaperCrawler",
        QString(
        "<h2>PaperCrawler Desktop</h2>"
        "<p>Version 2.0.0</p>"
        "<p>Academic paper search, management and analysis tool.</p>"
        "<hr>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Full-text search with pagination and caching</li>"
        "<li>CCF level / year / type filters</li>"
        "<li>Favorites with notes (%1 saved)</li>"
        "<li>Export to CSV / BibTeX / JSON / PDF</li>"
        "<li>Crawler task management</li>"
        "<li>AI research assistant (chat + review)</li>"
        "<li>Statistics dashboard</li>"
        "<li>Local SQLite database (%2 papers)</li>"
        "<li>Offline search fallback</li>"
        "<li>Dark theme support</li>"
        "<li>System tray integration</li>"
        "</ul>"
        "<hr>"
        "<p>Cache: %3 entries | Server: %4</p>"
        "<p>Built with Qt 6 / C++17</p>"
        "<p>&copy; 2026 PaperCrawler Project</p>"
        ).arg(favCount).arg(dbCount).arg(cacheSize).arg(apiManager_->baseUrl())
    );
}

void MainWindow::onPaperAdded(int paperId) {
    qDebug() << "Paper added to database:" << paperId;
}

void MainWindow::onDatabaseError(const QString& error) {
    qWarning() << "Database error:" << error;
    statusBar()->showMessage("数据库错误: " + error.left(50), 5000);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (trayManager_ && QSystemTrayIcon::isSystemTrayAvailable()) {
        hide();
        trayManager_->showNotification("PaperCrawler",
            "Running in background. Double-click tray icon to restore.");
        event->ignore();
    } else {
        saveSettings();
        event->accept();
    }
}

// ============================================================================
// Paper Details
// ============================================================================

void MainWindow::onPaperDetailsSuccess(const Paper& paper) {
    auto* favMgr = findChild<FavoriteManager*>();
    auto* dialog = new PaperDetailDialog(paper, favMgr, apiManager_, this);
    connect(dialog, &PaperDetailDialog::favoriteToggled, this, [this](int, bool) {
        if (tabWidget_->currentIndex() == 1) refreshFavoritesTab();
    });
    connect(dialog, &PaperDetailDialog::paperDeleted, this, [this](int paperId) {
        statusBar()->showMessage(QString("Paper %1 deleted").arg(paperId), 5000);
        ToastWidget::showInfo(QString("Paper %1 deleted").arg(paperId));
        if (!currentKeyword_.isEmpty()) {
            apiManager_->searchPapers(currentKeyword_, "", "", currentOffset_, currentLimit_);
        }
    });
    connect(dialog, &PaperDetailDialog::tagsChanged, this, [](const QStringList&) {
        ToastWidget::showSuccess("Tags updated");
    });

    // Record in recent history
    if (recentHistory_) {
        recentHistory_->addEntry(paper.id, paper.title, paper.authors, paper.year);
    }

    dialog->exec();
    dialog->deleteLater();
}

// ============================================================================
// Authentication Integration
// ============================================================================

void MainWindow::initializeAuthentication() {
    authManager_ = new AuthManager(this);
    authManager_->setBaseUrl(apiManager_->baseUrl());

    connect(authManager_, &AuthManager::authenticationChanged,
            this, &MainWindow::onAuthenticationChanged);
    connect(authManager_, &AuthManager::loginSuccess,
            this, &MainWindow::onLoginSuccess);
    connect(authManager_, &AuthManager::logoutSuccess,
            this, &MainWindow::onLogoutSuccess);

    if (authManager_->loadSavedState()) {
        qDebug() << "Restored session for user:" << authManager_->getCurrentUser().username;
        apiManager_->setAuthToken(authManager_->getAccessToken());
    }
}

void MainWindow::showLoginDialog() {
    if (!authManager_) {
        initializeAuthentication();
    }

    loginWindow_ = new LoginWindow(authManager_, this);
    connect(loginWindow_, &LoginWindow::authenticationSuccessful,
            this, &MainWindow::onLoginSuccess);
    loginWindow_->exec();
    loginWindow_->deleteLater();
    loginWindow_ = nullptr;
}

void MainWindow::handleLogout() {
    if (!authManager_) return;

    auto reply = QMessageBox::question(this, "Logout",
        "Are you sure you want to logout?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        authManager_->logout();
    }
}

void MainWindow::onLoginSuccess(const DesktopUser& user) {
    qDebug() << "Login successful:" << user.username;
    apiManager_->setAuthToken(authManager_->getAccessToken());
    updateAuthUI();
    ToastWidget::showSuccess(QString("Welcome, %1!").arg(user.username));
}

void MainWindow::onLogoutSuccess() {
    qDebug() << "Logout successful";
    apiManager_->clearAuthToken();
    updateAuthUI();
    ToastWidget::showInfo("Logged out");
}

void MainWindow::onAuthenticationChanged(bool authenticated) {
    qDebug() << "Auth state changed:" << authenticated;
    if (authenticated) {
        apiManager_->setAuthToken(authManager_->getAccessToken());
    }
    updateAuthUI();
}

void MainWindow::updateAuthUI() {
    if (!authManager_) return;

    bool authenticated = authManager_->isAuthenticated();

    if (loginAction_) loginAction_->setVisible(!authenticated);
    if (logoutAction_) logoutAction_->setVisible(authenticated);
    if (profileAction_) profileAction_->setVisible(authenticated);

    if (userLabel_) {
        if (authenticated) {
            DesktopUser user = authManager_->getCurrentUser();
            userLabel_->setText(QString("%1").arg(user.username));
            userLabel_->setToolTip(QString("Logged in as %1\n%2")
                .arg(user.fullName, user.email));
        } else {
            userLabel_->setText("");
        }
    }

    if (authenticated) {
        DesktopUser user = authManager_->getCurrentUser();
        setWindowTitle(QString("PaperCrawler - %1").arg(user.username));
    } else {
        setWindowTitle("PaperCrawler");
    }
}

// ============================================================================
// Favorites Tab Refresh
// ============================================================================

void MainWindow::refreshFavoritesTab() {
    auto* page = findChild<QWidget*>("favoritesPage");
    if (!page) return;

    auto* favMgr = findChild<FavoriteManager*>();
    if (!favMgr) return;

    auto* countLabel = page->findChild<QLabel*>("favCountLabel");
    auto* listLayout = page->findChild<QVBoxLayout*>("favListLayout");

    if (!listLayout) return;

    // Clear existing items
    QLayoutItem* item;
    while ((item = listLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    auto favorites = favMgr->getFavorites();

    if (countLabel) {
        countLabel->setText(QString("%1 paper(s) saved").arg(favorites.size()));
    }

    // Update tab badge
    if (tabWidget_) {
        tabWidget_->setTabText(1, QString("Favorites (%1)").arg(favorites.size()));
    }

    if (favorites.isEmpty()) {
        auto* emptyLabel = new QLabel("No favorites yet. Click the star on paper cards to add.");
        emptyLabel->setStyleSheet("color: palette(mid); font-size: 14px; padding: 40px;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        listLayout->addWidget(emptyLabel);
        return;
    }

    for (const auto& fav : favorites) {
        auto* card = new QWidget();
        card->setStyleSheet(
            "QWidget { background: palette(base); border: 1px solid palette(mid); "
            "border-radius: 8px; padding: 12px 16px; }"
        );

        auto* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(0, 0, 0, 0);
        cardLayout->setSpacing(4);

        auto* titleLabel = new QLabel(fav.title);
        titleLabel->setWordWrap(true);
        titleLabel->setStyleSheet("font-weight: bold; color: palette(text); font-size: 14px;");
        cardLayout->addWidget(titleLabel);

        // Notes display
        if (!fav.notes.isEmpty()) {
            auto* notesLabel = new QLabel(fav.notes.left(100));
            notesLabel->setWordWrap(true);
            notesLabel->setStyleSheet("color: palette(mid); font-size: 12px; font-style: italic; padding-left: 8px; border-left: 2px solid #e0e7ff;");
            cardLayout->addWidget(notesLabel);
        }

        auto* metaRow = new QHBoxLayout();
        auto* journalLabel = new QLabel(fav.journal);
        journalLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
        auto* yearLabel = new QLabel(fav.year);
        yearLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
        metaRow->addWidget(journalLabel);
        metaRow->addWidget(yearLabel);
        metaRow->addStretch();

        // Note button
        auto* noteBtn = new QPushButton("Note");
        noteBtn->setStyleSheet(
            "QPushButton { background: none; border: 1px solid palette(mid); color: palette(text); "
            "border-radius: 4px; padding: 2px 8px; font-size: 11px; }"
            "QPushButton:hover { background: #e0e7ff; }"
        );
        int pid = fav.paperId;
        connect(noteBtn, &QPushButton::clicked, this, [this, favMgr, pid]() {
            QString currentNote = favMgr->getNotes(pid);
            bool ok;
            QString note = QInputDialog::getMultiLineText(this, "Edit Note",
                "Add notes for this paper:", currentNote, &ok);
            if (ok) {
                favMgr->setNotes(pid, note);
                refreshFavoritesTab();
            }
        });
        metaRow->addWidget(noteBtn);

        auto* removeBtn = new QPushButton("Remove");
        removeBtn->setStyleSheet(
            "QPushButton { background: none; border: none; color: #ef4444; "
            "font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { color: #dc2626; }"
        );
        connect(removeBtn, &QPushButton::clicked, this, [this, favMgr, pid]() {
            favMgr->removeFavorite(pid);
        });
        metaRow->addWidget(removeBtn);
        cardLayout->addLayout(metaRow);

        listLayout->addWidget(card);
    }
}
