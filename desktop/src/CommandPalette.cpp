#include "CommandPalette.hpp"
#include <QVBoxLayout>
#include <QShortcut>
#include <QKeyEvent>

CommandPalette::CommandPalette(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setupUI();
}

void CommandPalette::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setFixedWidth(500);
    setMaximumHeight(400);

    auto* container = new QWidget();
    container->setStyleSheet(
        "QWidget { background: palette(window); border: 1px solid palette(mid); "
        "border-radius: 8px; }"
    );
    auto* innerLayout = new QVBoxLayout(container);
    innerLayout->setContentsMargins(8, 8, 8, 8);

    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Type a command...");
    searchEdit_->setStyleSheet(
        "QLineEdit { padding: 10px; font-size: 14px; border: 1px solid palette(mid); "
        "border-radius: 6px; background: palette(base); }"
    );
    connect(searchEdit_, &QLineEdit::textChanged, this, &CommandPalette::onSearchChanged);
    innerLayout->addWidget(searchEdit_);

    listWidget_ = new QListWidget();
    listWidget_->setStyleSheet(
        "QListWidget { border: none; background: transparent; }"
        "QListWidget::item { padding: 6px 10px; border-radius: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    listWidget_->setMaximumHeight(300);
    connect(listWidget_, &QListWidget::itemActivated, this, &CommandPalette::onItemActivated);
    innerLayout->addWidget(listWidget_);

    hintLabel_ = new QLabel("↑↓ navigate | Enter execute | Esc close");
    hintLabel_->setStyleSheet("color: palette(mid); font-size: 10px; padding: 4px;");
    innerLayout->addWidget(hintLabel_);

    layout->addWidget(container);
}

void CommandPalette::addAction(const QString& name, const QString& shortcut,
                                const QString& category,
                                const std::function<void()>& callback) {
    commands_.append({name, shortcut, category, callback});
}

void CommandPalette::showPalette() {
    searchEdit_->clear();
    refreshList("");
    show();
    searchEdit_->setFocus();

    if (parentWidget()) {
        QPoint center = parentWidget()->geometry().center();
        move(center.x() - width() / 2, center.y() - height() / 2);
    }
}

void CommandPalette::hidePalette() {
    hide();
    searchEdit_->clear();
}

void CommandPalette::onSearchChanged(const QString& text) {
    refreshList(text);
}

void CommandPalette::refreshList(const QString& filter) {
    listWidget_->clear();
    QString lower = filter.toLower();

    for (int i = 0; i < commands_.size(); ++i) {
        const auto& cmd = commands_[i];
        bool match = lower.isEmpty() ||
                     cmd.name.toLower().contains(lower) ||
                     cmd.category.toLower().contains(lower);

        if (match) {
            QString label = cmd.name;
            if (!cmd.shortcut.isEmpty()) {
                label += QString("  %1").arg(cmd.shortcut);
            }
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, i);
            if (!cmd.shortcut.isEmpty()) {
                item->setToolTip(cmd.shortcut);
            }
            listWidget_->addItem(item);
        }
    }

    if (listWidget_->count() > 0) {
        listWidget_->setCurrentRow(0);
    }
}

void CommandPalette::onItemActivated(QListWidgetItem* item) {
    if (!item) return;
    int idx = item->data(Qt::UserRole).toInt();
    if (idx >= 0 && idx < commands_.size()) {
        hidePalette();
        emit commandExecuted(commands_[idx].name);
        if (commands_[idx].callback) commands_[idx].callback();
    }
}

void CommandPalette::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hidePalette();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        auto* current = listWidget_->currentItem();
        if (current) onItemActivated(current);
        return;
    }
    if (event->key() == Qt::Key_Down) {
        int row = listWidget_->currentRow() + 1;
        if (row < listWidget_->count()) listWidget_->setCurrentRow(row);
        return;
    }
    if (event->key() == Qt::Key_Up) {
        int row = listWidget_->currentRow() - 1;
        if (row >= 0) listWidget_->setCurrentRow(row);
        return;
    }
    QWidget::keyPressEvent(event);
}
