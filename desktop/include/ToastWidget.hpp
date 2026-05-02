#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QList>

class ToastWidget : public QWidget {
    Q_OBJECT

public:
    enum Type { Info, Success, Warning, Error };

    static void show(const QString& message, Type type = Info, int durationMs = 3000);
    static void showSuccess(const QString& msg, int ms = 3000);
    static void showError(const QString& msg, int ms = 4000);
    static void showWarning(const QString& msg, int ms = 3500);
    static void showInfo(const QString& msg, int ms = 3000);

private:
    explicit ToastWidget(const QString& message, Type type, int durationMs, QWidget* parent = nullptr);

    void setupUI(const QString& message, Type type);
    void appear();
    void disappear();

    static QWidget* getRootWidget();
    static QList<ToastWidget*>& activeToasts();
    static void repositionAll(QWidget* root);

    QTimer* dismissTimer_{nullptr};
    int durationMs_;
};
