#include "tools/HeroWidget.hpp"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

HeroWidget::HeroWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupStyles();
}

void HeroWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 60, 40, 40);
    mainLayout->setSpacing(10);
    mainLayout->setAlignment(Qt::AlignCenter);

    // Title
    titleLabel_ = new QLabel("📚 论文检索平台", this);
    titleLabel_->setAlignment(Qt::AlignCenter);
    titleLabel_->setObjectName("heroTitle");

    // Subtitle
    subtitleLabel_ = new QLabel("快速搜索、分析和导出学术论文", this);
    subtitleLabel_->setAlignment(Qt::AlignCenter);
    subtitleLabel_->setObjectName("heroSubtitle");

    // Health Status Widget
    statusWidget_ = new QWidget(this);
    statusWidget_->setObjectName("statusWidget");
    auto* statusLayout = new QHBoxLayout(statusWidget_);
    statusLayout->setContentsMargins(12, 8, 12, 8);
    statusLayout->setSpacing(8);
    statusLayout->setAlignment(Qt::AlignCenter);

    statusDot_ = new QLabel(statusWidget_);
    statusDot_->setObjectName("statusDot");
    statusDot_->setFixedSize(8, 8);

    statusLabel_ = new QLabel("检查中...", statusWidget_);
    statusLabel_->setObjectName("statusLabel");

    statusLayout->addWidget(statusDot_);
    statusLayout->addWidget(statusLabel_);

    // Add to main layout
    mainLayout->addWidget(titleLabel_);
    mainLayout->addWidget(subtitleLabel_);
    mainLayout->addWidget(statusWidget_, 0, Qt::AlignCenter);

    // Add stretch at bottom
    mainLayout->addStretch();
}

void HeroWidget::setupStyles() {
    // Title styling - matching web frontend
    titleLabel_->setStyleSheet(
        "QLabel#heroTitle {"
        "  color: white;"
        "  font-size: 48pt;"
        "  font-weight: bold;"
        "  background: transparent;"
        "}"
    );

    // Subtitle styling
    subtitleLabel_->setStyleSheet(
        "QLabel#heroSubtitle {"
        "  color: rgba(255, 255, 255, 0.9);"
        "  font-size: 20pt;"
        "  background: transparent;"
        "}"
    );

    // Status widget styling
    statusWidget_->setStyleSheet(
        "QWidget#statusWidget {"
        "  background-color: rgba(255, 255, 255, 0.9);"
        "  border-radius: 20px;"
        "}"
    );

    statusLabel_->setStyleSheet(
        "QLabel#statusLabel {"
        "  font-size: 10pt;"
        "  font-weight: 600;"
        "  color: palette(text);"
        "  background: transparent;"
        "}"
    );

    // Apply initial status style
    QString dotStyle = getStatusStyle();
    statusDot_->setStyleSheet(dotStyle);

    // Create pulse animation for status dot
    auto* opacityEffect = new QGraphicsOpacityEffect(statusDot_);
    statusDot_->setGraphicsEffect(opacityEffect);

    auto* animation = new QPropertyAnimation(opacityEffect, "opacity", statusDot_);
    animation->setDuration(1000);
    animation->setStartValue(1.0);
    animation->setEndValue(0.5);
    animation->setLoopCount(-1);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}

void HeroWidget::setHealthStatus(HealthStatus status) {
    healthStatus_ = status;

    // Update status text
    statusLabel_->setText(getStatusText());

    // Update status dot style
    QString dotStyle = getStatusStyle();
    statusDot_->setStyleSheet(dotStyle);
}

QString HeroWidget::getStatusText() const {
    switch (healthStatus_) {
        case HealthStatus::Healthy:
            return "服务正常";
        case HealthStatus::Unhealthy:
            return "服务异常";
        case HealthStatus::Unknown:
        default:
            return "未知状态";
    }
}

QString HeroWidget::getStatusStyle() const {
    QString baseStyle = "QLabel#statusDot {"
                       "  border-radius: 4px;"
                       "}";

    QString color;
    switch (healthStatus_) {
        case HealthStatus::Healthy:
            color = "#10b981"; // Green
            break;
        case HealthStatus::Unhealthy:
            color = "#ef4444"; // Red
            break;
        case HealthStatus::Unknown:
        default:
            color = "#6b7280"; // Gray
            break;
    }

    return baseStyle + " background-color: " + color + ";";
}
