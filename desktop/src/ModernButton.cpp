#include "ModernButton.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QDebug>

ModernButton::ModernButton(QWidget* parent)
    : QPushButton(parent) {
    setupButton();
}

ModernButton::ModernButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent) {
    setupButton();
}

void ModernButton::setupButton() {
    setMinimumHeight(44);
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("QPushButton { border: none; }");

    setupShadow();
    setButtonStyle(ButtonStyle::Primary);
}

void ModernButton::setupShadow() {
    shadowEffect_ = new QGraphicsDropShadowEffect(this);
    shadowEffect_->setBlurRadius(15);
    shadowEffect_->setColor(QColor(0, 0, 0, 40));
    shadowEffect_->setOffset(0, 4);

    setGraphicsEffect(shadowEffect_);
}

void ModernButton::setButtonStyle(ButtonStyle style) {
    buttonStyle_ = style;

    switch (style) {
        case ButtonStyle::Primary:
            gradientStart_ = QColor(102, 126, 234);   // #667eea
            gradientEnd_ = QColor(118, 75, 162);      // #764ba2
            hoverStart_ = QColor(118, 75, 162);       // #764ba2
            hoverEnd_ = QColor(102, 126, 234);        // #667eea
            textColor_ = QColor(255, 255, 255);
            break;

        case ButtonStyle::Secondary:
            gradientStart_ = QColor(75, 85, 99);      // #4b5563
            gradientEnd_ = QColor(55, 65, 81);        // #374151
            hoverStart_ = QColor(55, 65, 81);         // #374151
            hoverEnd_ = QColor(75, 85, 99);           // #4b5563
            textColor_ = QColor(255, 255, 255);
            break;

        case ButtonStyle::Success:
            gradientStart_ = QColor(34, 197, 94);     // #22c55e
            gradientEnd_ = QColor(22, 163, 74);       // #16a34a
            hoverStart_ = QColor(22, 163, 74);        // #16a34a
            hoverEnd_ = QColor(34, 197, 94);          // #22c55e
            textColor_ = QColor(255, 255, 255);
            break;

        case ButtonStyle::Warning:
            gradientStart_ = QColor(251, 146, 60);    // #fb923c
            gradientEnd_ = QColor(249, 115, 22);      // #f97316
            hoverStart_ = QColor(249, 115, 22);       // #f97316
            hoverEnd_ = QColor(251, 146, 60);         // #fb923c
            textColor_ = QColor(255, 255, 255);
            break;

        case ButtonStyle::Danger:
            gradientStart_ = QColor(239, 68, 68);     // #ef4444
            gradientEnd_ = QColor(220, 38, 38);       // #dc2626
            hoverStart_ = QColor(220, 38, 38);        // #dc2626
            hoverEnd_ = QColor(239, 68, 68);          // #ef4444
            textColor_ = QColor(255, 255, 255);
            break;

        case ButtonStyle::Ghost:
            gradientStart_ = QColor(255, 255, 255, 0);
            gradientEnd_ = QColor(255, 255, 255, 0);
            hoverStart_ = QColor(243, 244, 246);
            hoverEnd_ = QColor(229, 231, 235);
            textColor_ = QColor(17, 24, 39);
            shadowEnabled_ = false;
            break;
    }

    update();
}

void ModernButton::setCornerRadius(int radius) {
    cornerRadius_ = radius;
    update();
}

void ModernButton::setShadowEnabled(bool enabled) {
    shadowEnabled_ = enabled;
    if (enabled) {
        setGraphicsEffect(shadowEffect_);
    } else {
        setGraphicsEffect(nullptr);
    }
}

void ModernButton::setAnimationDuration(int duration) {
    animationDuration_ = duration;
}

void ModernButton::setHoverEnabled(bool enabled) {
    hoverEnabled_ = enabled;
}

void ModernButton::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Determine gradient colors
    QColor start = isHovered_ ? hoverStart_ : gradientStart_;
    QColor end = isHovered_ ? hoverEnd_ : gradientEnd_;

    // Create gradient
    QLinearGradient gradient(rect().topLeft(), rect().topRight());
    gradient.setColorAt(0, start);
    gradient.setColorAt(1, end);

    // Create rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1),
                       cornerRadius_, cornerRadius_);

    // Fill background
    painter.fillPath(path, gradient);

    // Draw text
    painter.setPen(textColor_);
    QFont font = painter.font();
    font.setBold(true);
    font.setPointSize(11);
    painter.setFont(font);

    QRect textRect = rect().adjusted(0, -1, 0, 0);
    painter.drawText(textRect, Qt::AlignCenter, text());

    // Draw pressed effect
    if (isPressed_) {
        QPainterPath pressedPath;
        pressedPath.addRoundedRect(rect().adjusted(1, 1, -1, -1),
                                  cornerRadius_, cornerRadius_);
        painter.fillPath(pressedPath, QColor(0, 0, 0, 20));
    }
}

void ModernButton::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);

    if (!hoverEnabled_) return;

    isHovered_ = true;

    // Enhance shadow
    shadowEffect_->setBlurRadius(25);
    shadowEffect_->setOffset(0, 6);
    shadowEffect_->setColor(QColor(0, 0, 0, 60));

    update();
}

void ModernButton::leaveEvent(QEvent* event) {
    Q_UNUSED(event);

    if (!hoverEnabled_) return;

    isHovered_ = false;

    // Restore shadow
    shadowEffect_->setBlurRadius(15);
    shadowEffect_->setOffset(0, 4);
    shadowEffect_->setColor(QColor(0, 0, 0, 40));

    update();
}

void ModernButton::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);

    isPressed_ = true;

    // Reduce shadow on press
    shadowEffect_->setBlurRadius(10);
    shadowEffect_->setOffset(0, 2);

    update();
}

void ModernButton::mouseReleaseEvent(QMouseEvent* event) {
    QPushButton::mouseReleaseEvent(event);

    isPressed_ = false;

    // Restore shadow
    shadowEffect_->setBlurRadius(25);
    shadowEffect_->setOffset(0, 6);

    update();
}
