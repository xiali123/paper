#include "MainWindow.hpp"
#include "SearchWidget.hpp"
#include "ResultView.hpp"
#include "ProgressView.hpp"
#include "FilterPanel.hpp"
#include <QTimer>
#include <QCloseEvent>

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

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {

    setWindowTitle("📚 PaperCrawler - Academic Paper Search Tool");
    resize(1200, 800);

    setupUI();
    createMenus();
    createToolBar();
    connectSignals();
    loadSettings();

    statusBar()->showMessage("Ready - PaperCrawler Desktop v1.0", 3000);
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::setupUI() {
    // Create central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout
    auto* mainLayout = new QHBoxLayout(centralWidget);

    // Create splitter
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left panel - filters
    filterPanel_ = new FilterPanel(this);
    filterPanel_->setMaximumWidth(250);
    splitter->addWidget(filterPanel_);

    // Right panel - search and results
    QWidget* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);

    // Search widget
    searchWidget_ = new SearchWidget(this);
    rightLayout->addWidget(searchWidget_);

    // Results view
    resultView_ = new ResultView(this);
    rightLayout->addWidget(resultView_);

    // Progress view
    progressView_ = new ProgressView(this);
    rightLayout->addWidget(progressView_);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);
}

void MainWindow::createMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");

    QAction* exportAction = fileMenu->addAction("&Export Results");
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExport);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    // Edit menu
    QMenu* editMenu = menuBar()->addMenu("&Edit");

    QAction* preferencesAction = editMenu->addAction("&Preferences");
    connect(preferencesAction, &QAction::triggered, this, &MainWindow::onPreferences);

    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");

    QAction* themeAction = viewMenu->addAction("&Toggle Theme");
    connect(themeAction, &QAction::triggered, this, &MainWindow::onToggleTheme);

    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");

    QAction* statsAction = toolsMenu->addAction("&Statistics");
    connect(statsAction, &QAction::triggered, this, &MainWindow::onShowStatistics);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");

    QAction* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::createToolBar() {
    QToolBar* toolBar = addToolBar("Main Toolbar");
    toolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    QAction* searchAction = toolBar->addAction("🔍 Search");
    connect(searchAction, &QAction::triggered, this, [this]() {
        // Focus on search widget
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

void MainWindow::connectSignals() {
    connect(searchWidget_, &SearchWidget::searchRequested,
            this, &MainWindow::onSearch);

    connect(resultView_, &ResultView::paperSelected,
            this, &MainWindow::onPaperSelected);
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
        QMessageBox::warning(this, "Search",
                           "Please enter a keyword to search.");
        return;
    }

    statusBar()->showMessage("Searching for: " + keyword + "...");

    // Show progress
    progressView_->updateProgress(0, 100, "Searching...");

    // Store keyword for lambda capture
    QString searchKeyword = keyword;

    // Simulate search (replace with actual API call)
    QTimer::singleShot(1000, this, [this, searchKeyword]() {
        progressView_->updateProgress(50, 100, "Fetching results...");

        QTimer::singleShot(1000, this, [this, searchKeyword]() {
            progressView_->complete();
            statusBar()->showMessage("Search completed!", 3000);

            // Show demo results
            resultView_->setPaperCount(10);

            QMessageBox::information(this, "Search",
                "Demo search completed!\n\n"
                "Keyword: " + searchKeyword + "\n\n"
                "Full functionality will be available when\n"
                "connected to the backend API.\n\n"
                "For now, you can also use the web version:\n"
                "Run START-WEB.bat and visit http://localhost:5173");
        });
    });
}

void MainWindow::onPaperSelected(int paperId) {
    QMessageBox::information(this, "Paper Details",
        QString("Selected paper ID: %1\n\n"
                "Full paper details will be available when\n"
                "connected to the backend API.\n\n"
                "For now, use the web version:\n"
                "Run START-WEB.bat and visit http://localhost:5173").arg(paperId));
}

void MainWindow::onExport() {
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Results",
        QDir::homePath() + "/papers.csv",
        "CSV Files (*.csv);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        statusBar()->showMessage("Results exported to: " + fileName);
        QMessageBox::information(this, "Export",
            "Export functionality will be implemented with the backend API.\n\n"
            "Saved to: " + fileName);
    }
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
    static bool darkTheme = false;

    if (!darkTheme) {
        qApp->setStyle(QStyleFactory::create("Fusion"));
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
        darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        qApp->setPalette(darkPalette);

        statusBar()->showMessage("Dark theme enabled");
    } else {
        qApp->setPalette(QStyleFactory::create("")->standardPalette());
        statusBar()->showMessage("Light theme enabled");
    }

    darkTheme = !darkTheme;
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
