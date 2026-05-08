#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>

class ApiManager;
class QSettings;

class SystemTrayManager : public QObject {
    Q_OBJECT

public:
    explicit SystemTrayManager(QObject* parent = nullptr);

    void show();
    void showNotification(const QString& title, const QString& message,
                          QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information,
                          int durationMs = 3000);
    void setUnreadCount(int count);

signals:
    void showWindowRequested();
    void searchRequested(const QString& query);
    void quitRequested();

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onQuickSearch();

private:
    void setupTray();
    void updateIcon();

    QSystemTrayIcon* trayIcon_{nullptr};
    QMenu* trayMenu_{nullptr};
    QAction* showAction_{nullptr};
    QAction* searchAction_{nullptr};
    QAction* healthAction_{nullptr};
    QAction* quitAction_{nullptr};
    int unreadCount_{0};
    bool darkIcon_{false};
};
