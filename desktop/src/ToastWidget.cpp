#include "ToastWidget.hpp"
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

ToastWidget::ToastWidget(const QString& message, Type type, int durationMs, QWidget* parent)
    : QWidget(parent)
    , durationMs_(durationMs)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setupUI(message, type);
}

void ToastWidget::setupUI(const QString& message, Type type) {
    QString bgColor, borderColor, textColor, icon;
    switch (type) {
        case Success:
            bgColor = "#ecfdf5"; borderColor = "#059669"; textColor = "#065f46";
            icon = "✓"; break;
        case Error:
            bgColor = "#fef2f2"; borderColor = "#dc2626"; textColor = "#991b1b";
            icon = "✕"; break;
        case Warning:
            bgColor = "#fffbeb"; borderColor = "#d97706"; textColor = "#92400e";
            icon = "⚠"; break;
        default:
            bgColor = "#eff6ff"; borderColor = "#3b82f6"; textColor = "#1e40af";
            icon = "ℹ"; break;
    }

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto* container = new QWidget();
    container->setStyleSheet(QString(
        "QWidget { background: %1; border: 1px solid %2; border-radius: 8px; }"
    ).arg(bgColor, borderColor));

    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(16, 10, 16, 10);
    layout->setSpacing(10);

    auto* iconLabel = new QLabel(icon);
    iconLabel->setStyleSheet(QString(
        "font-size: 16px; font-weight: bold; color: %1; border: none; background: transparent;"
    ).arg(borderColor));
    iconLabel->setFixedWidth(20);
    layout->addWidget(iconLabel);

    auto* msgLabel = new QLabel(message);
    msgLabel->setStyleSheet(QString(
        "font-size: 13px; color: %1; border: none; background: transparent;"
    ).arg(textColor));
    msgLabel->setWordWrap(true);
    msgLabel->setMaximumWidth(400);
    layout->addWidget(msgLabel, 1);

    mainLayout->addWidget(container);
    adjustSize();
    setFixedHeight(height());
}

void ToastWidget::appear() {
    QPropertyAnimation* anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
    show();

    dismissTimer_ = new QTimer(this);
    dismissTimer_->setSingleShot(true);
    connect(dismissTimer_, &QTimer::timeout, this, &ToastWidget::disappear);
    dismissTimer_->start(durationMs_);
}

void ToastWidget::disappear() {
    QPropertyAnimation* anim = new QPropertyAnimation(this, "windowOpacity");
    anim->setDuration(200);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished, this, [this]() {
        activeToasts().removeOne(this);
        deleteLater();
        if (auto* root = getRootWidget()) repositionAll(root);
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

QWidget* ToastWidget::getRootWidget() {
    auto* app = QApplication::instance();
    for (auto* w : QApplication::topLevelWidgets()) {
        if (w->isWindow() && w->isVisible()) return w;
    }
    return nullptr;
}

QList<ToastWidget*>& ToastWidget::activeToasts() {
    static QList<ToastWidget*> toasts;
    return toasts;
}

void ToastWidget::repositionAll(QWidget* root) {
    QRect geo = root->geometry();
    int x = geo.x() + (geo.width() - 300) / 2;
    int y = geo.y() + 60;

    for (auto* toast : activeToasts()) {
        toast->move(x - (toast->width() - 300) / 2, y);
        y += toast->height() + 8;
    }
}

void ToastWidget::show(const QString& message, Type type, int durationMs) {
    auto* root = getRootWidget();
    auto* toast = new ToastWidget(message, type, durationMs, root);
    activeToasts().append(toast);
    if (root) repositionAll(root);
    toast->appear();
}

void ToastWidget::showSuccess(const QString& msg, int ms) { show(msg, Success, ms); }
void ToastWidget::showError(const QString& msg, int ms) { show(msg, Error, ms); }
void ToastWidget::showWarning(const QString& msg, int ms) { show(msg, Warning, ms); }
void ToastWidget::showInfo(const QString& msg, int ms) { show(msg, Info, ms); }
