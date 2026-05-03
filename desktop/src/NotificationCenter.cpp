#include "NotificationCenter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>

Notification Notification::create(Type t, const QString& title, const QString& msg,
                                   const QString& action) {
    Notification n;
    n.type = t;
    n.title = title;
    n.message = msg;
    n.action = action;
    n.timestamp = QDateTime::currentDateTime();
    return n;
}

NotificationCenter::NotificationCenter(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void NotificationCenter::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* header = new QHBoxLayout();
    header->setContentsMargins(8, 6, 8, 6);

    auto* titleLabel = new QLabel("Notifications");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    header->addWidget(titleLabel);

    countLabel_ = new QLabel("0");
    countLabel_->setStyleSheet(
        "background: #3b82f6; color: white; border-radius: 8px; "
        "padding: 1px 6px; font-size: 10px; font-weight: bold;"
    );
    header->addWidget(countLabel_);
    header->addStretch();

    markReadBtn_ = new QPushButton("Mark All Read");
    markReadBtn_->setStyleSheet("font-size: 10px; border: none; color: #3b82f6;");
    connect(markReadBtn_, &QPushButton::clicked, this, &NotificationCenter::onMarkAllRead);
    header->addWidget(markReadBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("font-size: 10px; border: none; color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &NotificationCenter::onClearAll);
    header->addWidget(clearBtn_);

    layout->addLayout(header);

    listWidget_ = new QListWidget();
    listWidget_->setStyleSheet(
        "QListWidget { border: none; background: palette(base); }"
        "QListWidget::item { padding: 8px; border-bottom: 1px solid palette(mid); }"
        "QListWidget::item:hover { background: palette(alternate-base); }"
    );
    connect(listWidget_, &QListWidget::itemClicked, this, &NotificationCenter::onItemClicked);
    layout->addWidget(listWidget_, 1);
    setMaximumWidth(320);
}

void NotificationCenter::addNotification(const Notification& n) {
    notifications_.prepend(n);
    if (notifications_.size() > MAX_NOTIFICATIONS) notifications_.removeLast();
    refreshList();
    emit unreadCountChanged(unreadCount());
}

void NotificationCenter::addNotification(Notification::Type type, const QString& title,
                                          const QString& message, const QString& action) {
    auto n = Notification::create(type, title, message, action);
    addNotification(n);
}

int NotificationCenter::unreadCount() const {
    int count = 0;
    for (const auto& n : notifications_) {
        if (!n.read) count++;
    }
    return count;
}

void NotificationCenter::clearAll() {
    notifications_.clear();
    refreshList();
    emit unreadCountChanged(0);
}

QString NotificationCenter::formatNotification(const Notification& n) const {
    QString icon;
    switch (n.type) {
        case Notification::Success: icon = "✓ "; break;
        case Notification::Warning: icon = "⚠ "; break;
        case Notification::Error:   icon = "✕ "; break;
        default:                     icon = "ℹ "; break;
    }
    QString time = n.timestamp.toString("HH:mm");
    QString style = n.read ? "color: palette(mid);" : "font-weight: bold;";
    return QString("%1%2\n%3%4").arg(icon, n.title, n.message.left(80),
                                      n.message.length() > 80 ? "..." : "");
}

void NotificationCenter::refreshList() {
    listWidget_->clear();
    for (int i = 0; i < notifications_.size(); ++i) {
        const auto& n = notifications_[i];
        auto* item = new QListWidgetItem(formatNotification(n));
        item->setData(Qt::UserRole, i);
        item->setData(Qt::UserRole + 1, n.action);

        QString fg = n.read ? "palette(mid)" : "palette(text)";
        QString iconColor;
        switch (n.type) {
            case Notification::Success: iconColor = "#059669"; break;
            case Notification::Warning: iconColor = "#d97706"; break;
            case Notification::Error:   iconColor = "#dc2626"; break;
            default:                     iconColor = "#3b82f6"; break;
        }
        item->setForeground(n.read ? palette().mid() : palette().text());
        listWidget_->addItem(item);
    }
    countLabel_->setText(QString::number(unreadCount()));
}

void NotificationCenter::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int idx = item->data(Qt::UserRole).toInt();
    if (idx >= 0 && idx < notifications_.size()) {
        notifications_[idx].read = true;
        QString action = item->data(Qt::UserRole + 1).toString();
        if (!action.isEmpty()) emit actionTriggered(action);
        refreshList();
        emit unreadCountChanged(unreadCount());
    }
}

void NotificationCenter::onClearAll() {
    clearAll();
}

void NotificationCenter::onMarkAllRead() {
    for (auto& n : notifications_) n.read = true;
    refreshList();
    emit unreadCountChanged(0);
}
