#include "core/SystemTrayManager.hpp"
#include <QApplication>
#include <QInputDialog>
#include <QSettings>

SystemTrayManager::SystemTrayManager(QObject* parent)
    : QObject(parent)
{
    setupTray();
}

void SystemTrayManager::setupTray() {
    trayMenu_ = new QMenu();

    showAction_ = trayMenu_->addAction("Show Window");
    showAction_->setIcon(QIcon::fromTheme("window"));
    connect(showAction_, &QAction::triggered, this, &SystemTrayManager::showWindowRequested);

    searchAction_ = trayMenu_->addAction("Quick Search");
    searchAction_->setIcon(QIcon::fromTheme("search"));
    connect(searchAction_, &QAction::triggered, this, &SystemTrayManager::onQuickSearch);

    trayMenu_->addSeparator();

    healthAction_ = trayMenu_->addAction("Health Check");
    connect(healthAction_, &QAction::triggered, this, []() {
        // Signal parent to do health check
    });

    trayMenu_->addSeparator();

    quitAction_ = trayMenu_->addAction("Quit");
    quitAction_->setIcon(QIcon::fromTheme("application-exit"));
    connect(quitAction_, &QAction::triggered, this, &SystemTrayManager::quitRequested);

    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setContextMenu(trayMenu_);
    trayIcon_->setToolTip("PaperCrawler");

    // Create a simple icon
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(59, 130, 246));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(2, 2, 28, 28, 6, 6);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "P");
    trayIcon_->setIcon(QIcon(pixmap));

    connect(trayIcon_, &QSystemTrayIcon::activated, this, &SystemTrayManager::onTrayActivated);
}

void SystemTrayManager::show() {
    if (trayIcon_) trayIcon_->show();
}

void SystemTrayManager::showNotification(const QString& title, const QString& message,
                                          QSystemTrayIcon::MessageIcon icon, int durationMs) {
    if (trayIcon_ && trayIcon_->isVisible()) {
        trayIcon_->showMessage(title, message, icon, durationMs);
    }
}

void SystemTrayManager::setUnreadCount(int count) {
    unreadCount_ = count;
    updateIcon();
}

void SystemTrayManager::updateIcon() {
    if (!trayIcon_) return;

    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(59, 130, 246));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(2, 2, 28, 28, 6, 6);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "P");

    // Badge
    if (unreadCount_ > 0) {
        QString badge = unreadCount_ > 99 ? "99+" : QString::number(unreadCount_);
        painter.setBrush(QColor(239, 68, 68));
        painter.setPen(Qt::NoPen);
        int badgeW = badge.length() > 2 ? 20 : 16;
        painter.drawRoundedRect(32 - badgeW - 2, 0, badgeW, 16, 8, 8);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        QRect badgeRect(32 - badgeW - 2, 0, badgeW, 16);
        painter.drawText(badgeRect, Qt::AlignCenter, badge);
    }

    trayIcon_->setIcon(QIcon(pixmap));
    trayIcon_->setToolTip(unreadCount_ > 0
        ? QString("PaperCrawler — %1 unread").arg(unreadCount_)
        : "PaperCrawler");
}

void SystemTrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        emit showWindowRequested();
    }
}

void SystemTrayManager::onQuickSearch() {
    bool ok;
    QString query = QInputDialog::getText(nullptr, "Quick Search",
        "Search papers:", QLineEdit::Normal, "", &ok);
    if (ok && !query.trimmed().isEmpty()) {
        emit searchRequested(query.trimmed());
    }
}
