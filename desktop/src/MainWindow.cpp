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
#include "AuthManager.hpp"
#include "LoginWindow.hpp"
#include "PaperDetailDialog.hpp"
#include "SettingsDialog.hpp"
#include "FavoriteManager.hpp"
#include "SearchHistory.hpp"
#include "database/LocalDatabase.hpp"
#include <QTimer>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QLinearGradient>
#include <QScrollArea>

#include <QMenuBar>
#include <QShortcut>
#include <QTextEdit>
#include <QTableWidget>
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

    // Check API health on startup
    apiManager_->checkHealth();

    // Periodic health check every 30 seconds
    auto* healthTimer = new QTimer(this);
    connect(healthTimer, &QTimer::timeout, this, [this]() {
        apiManager_->checkHealth();
    });
    healthTimer->start(30000);

    statusBar()->showMessage("Ready - PaperCrawler Desktop v1.0", 3000);

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

    searchLayout->addStretch();

    searchScroll->setWidget(searchPage);
    tabWidget_->addTab(searchScroll, "Search");

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

        tabWidget_->addTab(page, "Favorites");
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

        auto* taskTable = new QTableWidget(0, 5);
        taskTable->setObjectName("crawlerTaskTable");
        taskTable->setHorizontalHeaderLabels({"ID", "Source", "URL", "Status", "Created"});
        taskTable->horizontalHeader()->setStretchLastSection(true);
        taskTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        taskTable->setAlternatingRowColors(true);
        layout->addWidget(taskTable, 1);

        connect(apiManager_, &ApiManager::crawlerTasksSuccess, this,
            [taskTable](const QJsonArray& tasks) {
                taskTable->setRowCount(tasks.size());
                for (int i = 0; i < tasks.size(); ++i) {
                    auto t = tasks[i].toObject();
                    taskTable->setItem(i, 0, new QTableWidgetItem(QString::number(t["id"].toInt())));
                    taskTable->setItem(i, 1, new QTableWidgetItem(t["source"].toString()));
                    taskTable->setItem(i, 2, new QTableWidgetItem(t["url"].toString().left(60)));
                    taskTable->setItem(i, 3, new QTableWidgetItem(t["status"].toString()));
                    taskTable->setItem(i, 4, new QTableWidgetItem(t["created_at"].toString()));
                }
            });

        connect(apiManager_, &ApiManager::crawlerDashboardSuccess, this,
            [page](const QJsonObject& data) {
                auto updateCard = [page](const QString& name, const QString& value) {
                    auto* card = page->findChild<QWidget*>(name);
                    if (!card) return;
                    auto labels = card->findChildren<QLabel*>();
                    if (!labels.isEmpty()) labels[0]->setText(value);
                };
                updateCard("Total Tasks", QString::number(data["totalTasks"].toInt(data["total_tasks"].toInt())));
                updateCard("Running", QString::number(data["runningTasks"].toInt(data["running_tasks"].toInt())));
                updateCard("Completed", QString::number(data["completedTasks"].toInt(data["completed_tasks"].toInt())));
                updateCard("Failed", QString::number(data["failedTasks"].toInt(data["failed_tasks"].toInt())));
            });

        tabWidget_->addTab(page, "Crawler");
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

        tabWidget_->addTab(page, "AI");
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

        tabWidget_->addTab(page, "Statistics");
    }
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
            meta->setStyleSheet("font-size: 10px; color: #64748b;");
            layout->addWidget(meta);

            int pid = paper.id;
            connect(card, &QWidget::mousePressEvent, this, [this, pid](QMouseEvent*) {
                onPaperSelected(pid);
            });

            gridLayout->addWidget(card);
        }
    });

    // Filter panel
    if (filterPanel_) {
        connect(filterPanel_, &FilterPanel::filterChanged,
                this, [this](const QString& level, const QString& year) {
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
                break;
            case 4: // Statistics
                apiManager_->getStats("overview");
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

    // Log cache statistics
    qDebug() << "Cache statistics for" << currentKeyword_ << ":"
             << "Cached pages:" << paperCache_->getCacheCount(currentKeyword_)
             << "Total cache size:" << paperCache_->getCacheSize();
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
    healthLabel->setText(healthy ? "Online" : "Offline");

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
    if (!resultView_ || resultView_->paperCount() == 0) {
        QMessageBox::warning(this, "Export",
            "No papers to export.\nPlease search first.");
        return;
    }

    QString fileName = exportManager_->showSaveDialog(this, format);
    if (fileName.isEmpty()) return;

    statusBar()->showMessage("Exporting to: " + fileName + "...");
    QList<Paper> papers = resultView_->getPapers();

    bool success = false;
    switch (format) {
        case ExportFormat::CSV:
            success = exportManager_->exportToCSV(fileName, papers);
            break;
        case ExportFormat::BibTeX:
            success = exportManager_->exportToBibTeX(fileName, papers);
            break;
        case ExportFormat::JSON:
            success = exportManager_->exportToJSON(fileName, papers);
            break;
        case ExportFormat::PDF:
            success = exportManager_->exportToPDF(fileName, papers);
            break;
    }

    if (success) {
        statusBar()->showMessage(QString("Exported %1 papers to %2").arg(papers.size()).arg(fileName), 5000);
    }
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
    }
}

void MainWindow::onShowStatistics() {
    tabWidget_->setCurrentIndex(4);
    apiManager_->getStats("overview");
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
    if (trayIcon_ && trayIcon_->isVisible()) {
        hide();
        trayIcon_->showMessage("PaperCrawler", "Running in background. Double-click to restore.",
                               QSystemTrayIcon::Information, 2000);
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
    auto* dialog = new PaperDetailDialog(paper, favMgr, this);
    connect(dialog, &PaperDetailDialog::favoriteToggled, this, [this](int, bool) {
        if (tabWidget_->currentIndex() == 1) refreshFavoritesTab();
    });
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
}

void MainWindow::onLogoutSuccess() {
    qDebug() << "Logout successful";
    apiManager_->clearAuthToken();
    updateAuthUI();
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

        auto* metaRow = new QHBoxLayout();
        auto* journalLabel = new QLabel(fav.journal);
        journalLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
        auto* yearLabel = new QLabel(fav.year);
        yearLabel->setStyleSheet("color: palette(mid); font-size: 12px;");
        metaRow->addWidget(journalLabel);
        metaRow->addWidget(yearLabel);
        metaRow->addStretch();

        auto* removeBtn = new QPushButton("Remove");
        removeBtn->setStyleSheet(
            "QPushButton { background: none; border: none; color: #ef4444; "
            "font-size: 11px; font-weight: bold; }"
            "QPushButton:hover { color: #dc2626; }"
        );
        int pid = fav.paperId;
        connect(removeBtn, &QPushButton::clicked, this, [this, favMgr, pid]() {
            favMgr->removeFavorite(pid);
        });
        metaRow->addWidget(removeBtn);
        cardLayout->addLayout(metaRow);

        listLayout->addWidget(card);
    }
}
