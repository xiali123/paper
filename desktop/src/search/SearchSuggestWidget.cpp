#include "search/SearchSuggestWidget.hpp"
#include <QVBoxLayout>
#include <QKeyEvent>

SearchSuggestWidget::SearchSuggestWidget(QLineEdit* parentEdit, QWidget* parent)
    : QWidget(parent, Qt::Popup)
    , edit_(parentEdit)
{
    setupUI();

    debounceTimer_ = new QTimer(this);
    debounceTimer_->setInterval(DEBOUNCE_MS);
    debounceTimer_->setSingleShot(true);
    connect(debounceTimer_, &QTimer::timeout, this, &SearchSuggestWidget::onTextChanged);

    if (edit_) {
        connect(edit_, &QLineEdit::textChanged, this, [this]() {
            debounceTimer_->start();
        });
    }
}

void SearchSuggestWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setStyleSheet(
        "QWidget { background: palette(window); border: 1px solid palette(mid); border-radius: 4px; }"
    );

    listWidget_ = new QListWidget();
    listWidget_->setStyleSheet(
        "QListWidget { border: none; }"
        "QListWidget::item { padding: 6px 12px; }"
        "QListWidget::item:hover { background: palette(highlight); color: palette(highlighted-text); }"
    );
    listWidget_->setMaximumHeight(200);
    connect(listWidget_, &QListWidget::itemClicked, this, &SearchSuggestWidget::onSuggestionClicked);
    layout->addWidget(listWidget_);
}

void SearchSuggestWidget::setSuggestions(const QStringList& items) {
    suggestions_ = items;
}

void SearchSuggestWidget::setHistory(const QStringList& history) {
    history_ = history;
}

void SearchSuggestWidget::onTextChanged() {
    if (!edit_) return;
    QString text = edit_->text().trimmed();

    listWidget_->clear();

    // Show history first
    if (!text.isEmpty()) {
        for (const auto& h : history_) {
            if (h.toLower().startsWith(text.toLower()) && h != text) {
                auto* item = new QListWidgetItem("H " + h);
                item->setData(Qt::UserRole, h);
                listWidget_->addItem(item);
            }
        }
    } else {
        for (const auto& h : history_.mid(0, 5)) {
            auto* item = new QListWidgetItem("H " + h);
            item->setData(Qt::UserRole, h);
            listWidget_->addItem(item);
        }
    }

    // API suggestions
    for (const auto& s : suggestions_) {
        if (text.isEmpty() || s.toLower().contains(text.toLower())) {
            auto* item = new QListWidgetItem(s);
            item->setData(Qt::UserRole, s);
            listWidget_->addItem(item);
        }
    }

    if (listWidget_->count() > 0) {
        showPopup();
    } else {
        hidePopup();
    }
}

void SearchSuggestWidget::onSuggestionClicked(QListWidgetItem* item) {
    if (!item) return;
    QString text = item->data(Qt::UserRole).toString();
    if (edit_) edit_->setText(text);
    emit suggestionSelected(text);
    hidePopup();
}

void SearchSuggestWidget::showPopup() {
    if (!edit_) return;
    QPoint pos = edit_->mapToGlobal(QPoint(0, edit_->height()));
    move(pos);
    setFixedWidth(edit_->width());
    show();
}

void SearchSuggestWidget::hidePopup() {
    hide();
}
