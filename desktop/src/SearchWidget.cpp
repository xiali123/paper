#include "SearchWidget.hpp"
#include "SearchHistory.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QTimer>
#include <QFrame>

SearchWidget::SearchWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupStyles();
}

void SearchWidget::setupUI() {
    // Main container with margin - vertical layout for suggestions
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(60, 20, 60, 40);
    mainLayout->setSpacing(20);

    // Search container widget
    auto* searchContainer = new QWidget(this);
    auto* containerLayout = new QHBoxLayout(searchContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(12);

    // Search input
    keywordEdit_ = new QLineEdit(this);
    keywordEdit_->setPlaceholderText("🔍 输入关键词，例如：deep learning, computer vision...");
    keywordEdit_->setMinimumHeight(50);
    keywordEdit_->setObjectName("searchInput");

    // Search button
    searchButton_ = new QPushButton("搜索", this);
    searchButton_->setMinimumHeight(50);
    searchButton_->setMinimumWidth(120);
    searchButton_->setObjectName("searchButton");
    searchButton_->setCursor(Qt::PointingHandCursor);

    containerLayout->addWidget(keywordEdit_);
    containerLayout->addWidget(searchButton_);

    // Suggestions section
    suggestionsLabel_ = new QLabel("热门搜索：", this);
    suggestionsLabel_->setObjectName("suggestionsLabel");
    suggestionsLabel_->setAlignment(Qt::AlignCenter);

    auto* suggestionsLayout = new QHBoxLayout();
    suggestionsLayout->setSpacing(15);
    suggestionsLayout->setAlignment(Qt::AlignCenter);

    // Create suggestion buttons
    QStringList suggestions = {"machine learning", "computer vision", "natural language processing"};
    for (const auto& suggestion : suggestions) {
        auto* btn = new QPushButton(suggestion, this);
        btn->setObjectName("suggestionButton");
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, &SearchWidget::onSuggestionClicked);
        suggestionButtons_.append(btn);
        suggestionsLayout->addWidget(btn);
    }

    // Status label
    statusLabel_ = new QLabel(this);
    statusLabel_->setObjectName("statusLabel");
    statusLabel_->setVisible(false);
    statusLabel_->setAlignment(Qt::AlignCenter);

    // Add to main layout
    mainLayout->addWidget(searchContainer);
    mainLayout->addWidget(suggestionsLabel_);
    mainLayout->addLayout(suggestionsLayout);
    mainLayout->addWidget(statusLabel_);

    // Connect signals
    connect(searchButton_, &QPushButton::clicked,
            this, &SearchWidget::onSearchClicked);
    connect(keywordEdit_, &QLineEdit::returnPressed,
            this, &SearchWidget::onSearchClicked);
    connect(keywordEdit_, &QLineEdit::textChanged,
            this, &SearchWidget::onTextChanged);

    // History dropdown (hidden by default)
    historyDropdown_ = new QListWidget(this);
    historyDropdown_->setObjectName("historyDropdown");
    historyDropdown_->setMaximumHeight(200);
    historyDropdown_->hide();
    historyDropdown_->setStyleSheet(
        "QListWidget#historyDropdown {"
        "  background: white; border: 1px solid #e2e8f0; border-radius: 8px; "
        "  padding: 4px; font-size: 11pt; }"
        "QListWidget#historyDropdown::item {"
        "  padding: 8px 16px; border-radius: 4px; }"
        "QListWidget#historyDropdown::item:hover {"
        "  background: #f1f5f9; }"
        "QListWidget#historyDropdown::item:selected {"
        "  background: #e0e7ff; color: #4f46e5; }"
    );
    connect(historyDropdown_, &QListWidget::itemClicked,
            this, &SearchWidget::onHistoryItemClicked);
}

void SearchWidget::setupStyles() {
    // Search Input Styling
    keywordEdit_->setStyleSheet(
        "QLineEdit#searchInput {"
        "  background-color: rgba(255, 255, 255, 0.95);"
        "  border: 2px solid #e5e7eb;"
        "  border-radius: 12px;"
        "  padding: 12px 20px;"
        "  font-size: 11pt;"
        "  color: #111827;"
        "  font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif;"
        "}"
        "QLineEdit#searchInput:focus {"
        "  border: 2px solid #6366f1;"
        "  background-color: rgba(255, 255, 255, 1.0);"
        "}"
    );

    // Search Button Styling
    searchButton_->setStyleSheet(
        "QPushButton#searchButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "    stop:0 #667eea, stop:1 #764ba2);"
        "  color: white;"
        "  border: none;"
        "  border-radius: 12px;"
        "  font-weight: 600;"
        "  font-size: 11pt;"
        "  font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif;"
        "}"
        "QPushButton#searchButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "    stop:0 #764ba2, stop:1 #667eea);"
        "}"
        "QPushButton#searchButton:pressed {"
        "  padding: 11px 19px;"
        "}"
    );

    // Status Label Styling
    statusLabel_->setStyleSheet(
        "QLabel#statusLabel {"
        "  color: #6b7280;"
        "  font-size: 10pt;"
        "  padding: 8px 16px;"
        "  background: rgba(255, 255, 255, 0.8);"
        "  border-radius: 8px;"
        "}"
    );

    // Suggestions Label Styling
    suggestionsLabel_->setStyleSheet(
        "QLabel#suggestionsLabel {"
        "  color: rgba(255, 255, 255, 0.9);"
        "  font-size: 10pt;"
        "  background: transparent;"
        "}"
    );

    // Suggestion Buttons Styling
    for (auto* btn : suggestionButtons_) {
        btn->setStyleSheet(
            "QPushButton#suggestionButton {"
            "  background-color: transparent;"
            "  color: #667eea;"
            "  border: none;"
            "  font-size: 10pt;"
            "  text-decoration: none;"
            "  padding: 4px 8px;"
            "}"
            "QPushButton#suggestionButton:hover {"
            "  color: #764ba2;"
            "  text-decoration: underline;"
            "}"
        );
    }
}

void SearchWidget::setPlaceholder(const QString& text) {
    keywordEdit_->setPlaceholderText(text);
}

void SearchWidget::setFocus() {
    keywordEdit_->setFocus();
}

void SearchWidget::onSearchClicked() {
    QString keyword = keywordEdit_->text().trimmed();
    if (!keyword.isEmpty()) {
        emit searchRequested(keyword);
        statusLabel_->setText("⏳ 搜索中...");
        statusLabel_->setVisible(true);

        // Auto-hide status after 3 seconds
        QTimer::singleShot(3000, this, [this]() {
            statusLabel_->setVisible(false);
        });
    }
}

void SearchWidget::onSuggestionClicked() {
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        keywordEdit_->setText(btn->text());
        emit searchRequested(btn->text());
        statusLabel_->setText("⏳ 搜索中...");
        statusLabel_->setVisible(true);

        QTimer::singleShot(3000, this, [this]() {
            statusLabel_->setVisible(false);
        });
    }
}

void SearchWidget::setSearchHistory(SearchHistory* history) {
    searchHistory_ = history;
}

void SearchWidget::onTextChanged(const QString& text) {
    if (!searchHistory_ || text.trimmed().isEmpty()) {
        hideHistoryDropdown();
        return;
    }
    showHistoryDropdown();
}

void SearchWidget::showHistoryDropdown() {
    if (!searchHistory_) return;

    QString prefix = keywordEdit_->text().trimmed().toLower();
    QStringList allHistory = searchHistory_->getRecentKeywords(20);

    QStringList filtered;
    for (const QString& kw : allHistory) {
        if (kw.toLower().contains(prefix)) {
            filtered.append(kw);
        }
        if (filtered.size() >= 8) break;
    }

    if (filtered.isEmpty()) {
        hideHistoryDropdown();
        return;
    }

    historyDropdown_->clear();
    for (const QString& kw : filtered) {
        historyDropdown_->addItem(kw);
    }

    QPoint pos = keywordEdit_->mapTo(this, QPoint(0, keywordEdit_->height()));
    historyDropdown_->setGeometry(
        pos.x(), pos.y(),
        keywordEdit_->width(), historyDropdown_->sizeHintForRow(0) * filtered.size() + 8
    );
    historyDropdown_->raise();
    historyDropdown_->show();
}

void SearchWidget::hideHistoryDropdown() {
    if (historyDropdown_) {
        historyDropdown_->hide();
    }
}

void SearchWidget::onHistoryItemClicked(QListWidgetItem* item) {
    if (item) {
        keywordEdit_->setText(item->text());
        hideHistoryDropdown();
        emit searchRequested(item->text());
    }
}
