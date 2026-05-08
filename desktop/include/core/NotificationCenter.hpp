#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QDateTime>

struct Notification {
    enum Type { Info, Success, Warning, Error };
    Type type{Info};
    QString title;
    QString message;
    QString action;     // e.g. "openPaper:123"
    QDateTime timestamp;
    bool read{false};

    static Notification create(Type t, const QString& title, const QString& msg,
                               const QString& action = "");
};

class NotificationCenter : public QWidget {
    Q_OBJECT

public:
    explicit NotificationCenter(QWidget* parent = nullptr);

    void addNotification(const Notification& n);
    void addNotification(Notification::Type type, const QString& title,
                         const QString& message, const QString& action = "");
    int unreadCount() const;
    void clearAll();

signals:
    void actionTriggered(const QString& action);
    void unreadCountChanged(int count);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onClearAll();
    void onMarkAllRead();

private:
    void setupUI();
    void refreshList();
    QString formatNotification(const Notification& n) const;

    QListWidget* listWidget_{nullptr};
    QLabel* countLabel_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QPushButton* markReadBtn_{nullptr};

    QList<Notification> notifications_;
    static constexpr int MAX_NOTIFICATIONS = 100;
};
