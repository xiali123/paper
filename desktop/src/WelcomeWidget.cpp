#include "WelcomeWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>

WelcomeWidget::WelcomeWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void WelcomeWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);

    // Welcome header
    welcomeLabel_ = new QLabel("Welcome to PaperCrawler");
    welcomeLabel_->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: palette(text);"
    );
    layout->addWidget(welcomeLabel_);

    auto* subtitleLabel = new QLabel(
        "Your academic paper management and research tool.\n"
        "Search, organize, annotate, and collaborate on research papers."
    );
    subtitleLabel->setStyleSheet("font-size: 14px; color: palette(mid);");
    subtitleLabel->setWordWrap(true);
    layout->addWidget(subtitleLabel);

    // Quick actions
    auto* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(12);

    searchBtn_ = new QPushButton("Start Search");
    searchBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border-radius: 8px; "
        "padding: 12px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(searchBtn_, &QPushButton::clicked, this, [this]() {
        emit searchRequested("");
    });
    actionLayout->addWidget(searchBtn_);

    auto* crawlerBtn = new QPushButton("Crawler");
    crawlerBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); "
        "border: 1px solid palette(mid); border-radius: 8px; padding: 12px 24px; font-size: 14px; }"
        "QPushButton:hover { background: palette(alternate-base); }"
    );
    connect(crawlerBtn, &QPushButton::clicked, this, [this]() { emit openTabRequested(2); });
    actionLayout->addWidget(crawlerBtn);

    auto* latexBtn = new QPushButton("LaTeX Editor");
    latexBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); "
        "border: 1px solid palette(mid); border-radius: 8px; padding: 12px 24px; font-size: 14px; }"
        "QPushButton:hover { background: palette(alternate-base); }"
    );
    connect(latexBtn, &QPushButton::clicked, this, [this]() { emit openTabRequested(7); });
    actionLayout->addWidget(latexBtn);

    tourBtn_ = new QPushButton("Take a Tour");
    tourBtn_->setStyleSheet(
        "QPushButton { background: transparent; color: #3b82f6; border: 2px solid #3b82f6; "
        "border-radius: 8px; padding: 12px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #eff6ff; }"
    );
    connect(tourBtn_, &QPushButton::clicked, this, &WelcomeWidget::tourRequested);
    actionLayout->addWidget(tourBtn_);

    actionLayout->addStretch();
    layout->addLayout(actionLayout);

    // Two columns: recent papers + tips
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(24);

    // Recent papers
    auto* recentPanel = new QWidget();
    auto* recentLayout = new QVBoxLayout(recentPanel);
    recentLayout->setContentsMargins(0, 0, 0, 0);

    auto* recentHeader = new QLabel("Recent Papers");
    recentHeader->setStyleSheet("font-weight: bold; font-size: 14px;");
    recentLayout->addWidget(recentHeader);

    recentList_ = new QListWidget();
    recentList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 8px; "
        "padding: 4px; background: palette(base); }"
        "QListWidget::item { padding: 8px; border-radius: 4px; }"
        "QListWidget::item:hover { background: palette(alternate-base); }"
    );
    connect(recentList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (item) emit openPaperRequested(item->data(Qt::UserRole).toInt());
    });
    recentLayout->addWidget(recentList_);
    contentLayout->addWidget(recentPanel, 1);

    // Tips
    auto* tipPanel = new QWidget();
    auto* tipLayout = new QVBoxLayout(tipPanel);
    tipLayout->setContentsMargins(0, 0, 0, 0);

    auto* tipHeader = new QLabel("Tips & Shortcuts");
    tipHeader->setStyleSheet("font-weight: bold; font-size: 14px;");
    tipLayout->addWidget(tipHeader);

    tipList_ = new QListWidget();
    tipList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 8px; "
        "padding: 4px; background: palette(base); }"
        "QListWidget::item { padding: 6px; border-radius: 4px; color: palette(text); }"
    );
    tipList_->addItem("Ctrl+Enter — Compile LaTeX");
    tipList_->addItem("Ctrl+Shift+F — Advanced Search");
    tipList_->addItem("Ctrl+1-8 — Switch tabs");
    tipList_->addItem("Ctrl+/ — Toggle comment in LaTeX");
    tipList_->addItem("Ctrl+F — Find & Replace");
    tipList_->addItem("Ctrl+H — Recent History");
    tipList_->addItem("Double-click paper — Open details");
    tipList_->addItem("Right-click — Context menu");
    tipLayout->addWidget(tipList_);
    contentLayout->addWidget(tipPanel, 1);

    layout->addLayout(contentLayout, 1);
}

void WelcomeWidget::setRecentPapers(const QList<QPair<int, QString>>& papers) {
    recentList_->clear();
    for (const auto& [id, title] : papers) {
        auto* item = new QListWidgetItem(title);
        item->setData(Qt::UserRole, id);
        recentList_->addItem(item);
    }
}

void WelcomeWidget::setTip(const QString& tip) {
    Q_UNUSED(tip);
}
