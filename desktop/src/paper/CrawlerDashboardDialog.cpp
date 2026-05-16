#include "paper/CrawlerDashboardDialog.hpp"

// ============================================================================
// CrawlerDashboardDialog
// ============================================================================

CrawlerDashboardDialog::CrawlerDashboardDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Crawler Dashboard");
    setMinimumSize(800, 600);
    setModal(true);
    setupUI();
}

void CrawlerDashboardDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* tabs = new QTabWidget();
    tabs->setStyleSheet(
        "QTabBar::tab { padding: 10px 24px; font-weight: bold; }"
        "QTabBar::tab:selected { color: #4f46e5; border-bottom: 2px solid #4f46e5; }"
    );

    tabs->addTab(createDashboardTab(), "Dashboard");
    tabs->addTab(createTasksTab(), "Tasks");
    tabs->addTab(createTemplatesTab(), "Templates");

    layout->addWidget(tabs);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
        "QPushButton:hover { background: palette(light); }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}

QWidget* CrawlerDashboardDialog::createDashboardTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    auto* titleLabel = new QLabel("Crawler Dashboard");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: palette(text);");
    layout->addWidget(titleLabel);

    auto* desc = new QLabel(
        "Monitor crawler tasks, manage templates, and view statistics.\n\n"
        "Features:\n"
        "- Real-time task monitoring\n"
        "- Template management (arXiv, PubMed, Scholar, IEEE, ACM)\n"
        "- Task scheduling and retry\n"
        "- Node management\n"
        "- Distributed crawler support"
    );
    desc->setStyleSheet("color: palette(mid); font-size: 14px; line-height: 1.6;");
    layout->addWidget(desc);
    layout->addStretch();

    return widget;
}

QWidget* CrawlerDashboardDialog::createTasksTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    auto* table = new QTableWidget(0, 5);
    table->setHorizontalHeaderLabels({"ID", "Source", "Status", "Papers", "Created"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table);

    return widget;
}

QWidget* CrawlerDashboardDialog::createTemplatesTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    auto* label = new QLabel("Crawler Templates");
    label->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
    layout->addWidget(label);

    auto* desc = new QLabel("Manage crawler templates for different data sources.");
    desc->setStyleSheet("color: palette(mid);");
    layout->addWidget(desc);
    layout->addStretch();

    return widget;
}

// ============================================================================
// AIDialog
// ============================================================================

AIDialog::AIDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("AI Research Assistant");
    setMinimumSize(800, 600);
    setModal(true);
    setupUI();
}

void AIDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* tabs = new QTabWidget();
    tabs->setStyleSheet(
        "QTabBar::tab { padding: 10px 24px; font-weight: bold; }"
        "QTabBar::tab:selected { color: #4f46e5; border-bottom: 2px solid #4f46e5; }"
    );

    tabs->addTab(createReviewTab(), "AI Review");
    tabs->addTab(createChatTab(), "Chat");
    tabs->addTab(createHistoryTab(), "History");

    layout->addWidget(tabs);

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
        "QPushButton:hover { background: palette(light); }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}

QWidget* AIDialog::createReviewTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    auto* titleLabel = new QLabel("AI Paper Review");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: palette(text);");
    layout->addWidget(titleLabel);

    auto* desc = new QLabel(
        "AI-powered paper review and analysis.\n\n"
        "Features:\n"
        "- Quick review / Detailed review / Peer review\n"
        "- Literature review generation\n"
        "- Research plan generation\n"
        "- Review history and statistics"
    );
    desc->setStyleSheet("color: palette(mid); font-size: 14px; line-height: 1.6;");
    layout->addWidget(desc);
    layout->addStretch();

    return widget;
}

QWidget* AIDialog::createChatTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    auto* label = new QLabel("AI Chat Assistant");
    label->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(text);");
    layout->addWidget(label);

    auto* desc = new QLabel("Chat with AI about papers, research topics, and more.");
    desc->setStyleSheet("color: palette(mid);");
    layout->addWidget(desc);
    layout->addStretch();

    return widget;
}

QWidget* AIDialog::createHistoryTab() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    auto* table = new QTableWidget(0, 4);
    table->setHorizontalHeaderLabels({"Type", "Subject", "Status", "Date"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(table);

    return widget;
}
