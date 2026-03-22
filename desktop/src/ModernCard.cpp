#include "ModernCard.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QDebug>

ModernCard::ModernCard(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    setupShadow();
    setupAnimations();
}

ModernCard::ModernCard(const QString& title, QWidget* parent)
    : QWidget(parent) {
    setupUI();
    setupShadow();
    setupAnimations();
    setTitle(title);
}

void ModernCard::setupUI() {
    setObjectName("ModernCard");

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(16, 16, 16, 16);
    layout_->setSpacing(12);

    // Title label
    titleLabel_ = new QLabel(this);
    titleLabel_->setObjectName("cardTitle");
    titleLabel_->setStyleSheet(
        "QLabel {"
        "  font-size: 14pt;"
        "  font-weight: 600;"
        "  color: #111827;"
        "  background: transparent;"
        "}"
    );
    layout_->addWidget(titleLabel_);

    // Content widget container
    contentWidget_ = new QWidget(this);
    layout_->addWidget(contentWidget_);

    // Set minimum size
    setMinimumHeight(80);
}

void ModernCard::setupShadow() {
    shadowEffect_ = new QGraphicsDropShadowEffect(this);
    shadowEffect_->setBlurRadius(20);
    shadowEffect_->setColor(QColor(0, 0, 0, 60));
    shadowEffect_->setOffset(0, 4);

    setGraphicsEffect(shadowEffect_);
}

void ModernCard::setupAnimations() {
    hoverAnimation_ = new QPropertyAnimation(this, "geometry");
    hoverAnimation_->setDuration(200);
    hoverAnimation_->setEasingCurve(QEasingCurve::OutCubic);
}

void ModernCard::setTitle(const QString& title) {
    titleLabel_->setText(title);
    titleLabel_->setVisible(!title.isEmpty());
}

void ModernCard::setContent(QWidget* content) {
    // Clear existing content
    if (contentWidget_->layout()) {
        delete contentWidget_->layout();
    }

    // Add new content
    auto* contentLayout = new QVBoxLayout(contentWidget_);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->addWidget(content);
}

void ModernCard::setCornerRadius(int radius) {
    cornerRadius_ = radius;
    update();
}

void ModernCard::setShadowEnabled(bool enabled) {
    shadowEnabled_ = enabled;
    if (enabled) {
        setGraphicsEffect(shadowEffect_);
    } else {
        setGraphicsEffect(nullptr);
    }
}

void ModernCard::setHoverEnabled(bool enabled) {
    hoverEnabled_ = enabled;
}

void ModernCard::setCardColor(const QColor& color) {
    cardColor_ = color;
    update();
}

void ModernCard::setBorderColor(const QColor& color) {
    borderColor_ = color;
    update();
}

void ModernCard::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Create rounded rectangle path
    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1),
                       cornerRadius_, cornerRadius_);

    // Fill background with glass effect
    painter.fillPath(path, cardColor_);

    // Draw border
    QPen borderPen(borderColor_, 1);
    painter.setPen(borderPen);
    painter.drawPath(path);
}

void ModernCard::enterEvent(QEnterEvent* event) {
    Q_UNUSED(event);

    if (!hoverEnabled_) return;

    isHovered_ = true;

    // Enhance shadow on hover
    shadowEffect_->setBlurRadius(30);
    shadowEffect_->setOffset(0, 8);
    shadowEffect_->setColor(QColor(0, 0, 0, 100));

    // Scale animation effect
    update();
}

void ModernCard::leaveEvent(QEvent* event) {
    Q_UNUSED(event);

    if (!hoverEnabled_) return;

    isHovered_ = false;

    // Restore shadow
    shadowEffect_->setBlurRadius(20);
    shadowEffect_->setOffset(0, 4);
    shadowEffect_->setColor(QColor(0, 0, 0, 60));

    update();
}
