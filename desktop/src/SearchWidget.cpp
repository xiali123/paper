#include "SearchWidget.hpp"
#include <QHBoxLayout>

SearchWidget::SearchWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void SearchWidget::setupUI() {
    auto* layout = new QHBoxLayout(this);

    keywordEdit_ = new QLineEdit(this);
    keywordEdit_->setPlaceholderText("Enter search keyword...");
    keywordEdit_->setMinimumWidth(400);

    searchButton_ = new QPushButton("Search", this);
    searchButton_->setMinimumWidth(100);

    statusLabel_ = new QLabel(this);

    layout->addWidget(keywordEdit_);
    layout->addWidget(searchButton_);
    layout->addWidget(statusLabel_);
    layout->addStretch();

    connect(searchButton_, &QPushButton::clicked,
            this, &SearchWidget::onSearchClicked);
    connect(keywordEdit_, &QLineEdit::returnPressed,
            this, &SearchWidget::onSearchClicked);
}

void SearchWidget::onSearchClicked() {
    QString keyword = keywordEdit_->text().trimmed();
    if (!keyword.isEmpty()) {
        emit searchRequested(keyword);
        statusLabel_->setText("Searching...");
    }
}
