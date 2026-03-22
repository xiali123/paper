#include "FeatureCards.hpp"
#include <QPropertyAnimation>
#include <QEasingCurve>

FeatureCards::FeatureCards(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupStyles();
}

void FeatureCards::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 60);
    mainLayout->setSpacing(20);

    // Grid layout for cards
    gridLayout_ = new QGridLayout();
    gridLayout_->setHorizontalSpacing(20);
    gridLayout_->setVerticalSpacing(20);

    // Create 4 feature cards
    auto* card1 = createFeatureCard("🔍", "论文搜索", "从DBLP数据库快速检索学术论文");
    auto* card2 = createFeatureCard("📊", "统计分析", "期刊分布、年度趋势分析");
    auto* card3 = createFeatureCard("📥", "数据导出", "支持CSV、JSON、BibTeX格式");
    auto* card4 = createFeatureCard("⚡", "高性能", "C++实现，性能提升10-100倍");

    // Add cards to grid (2x2 layout)
    gridLayout_->addWidget(card1, 0, 0);
    gridLayout_->addWidget(card2, 0, 1);
    gridLayout_->addWidget(card3, 1, 0);
    gridLayout_->addWidget(card4, 1, 1);

    // Set column stretch for equal width
    gridLayout_->setColumnStretch(0, 1);
    gridLayout_->setColumnStretch(1, 1);

    mainLayout->addLayout(gridLayout_);
}

void FeatureCards::setupStyles() {
    setStyleSheet(
        "FeatureCards {"
        "  background: transparent;"
        "}"
    );
}

QWidget* FeatureCards::createFeatureCard(const QString& icon, const QString& title,
                                         const QString& description) {
    auto* card = new QWidget();
    card->setObjectName("featureCard");
    card->setMinimumHeight(180);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);
    layout->setAlignment(Qt::AlignCenter);

    // Icon
    auto* iconLabel = new QLabel(icon, card);
    iconLabel->setObjectName("featureIcon");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setMinimumHeight(60);

    // Title
    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName("featureTitle");
    titleLabel->setAlignment(Qt::AlignCenter);

    // Description
    auto* descLabel = new QLabel(description, card);
    descLabel->setObjectName("featureDesc");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);

    // Apply styling
    card->setStyleSheet(
        "QWidget#featureCard {"
        "  background-color: rgba(255, 255, 255, 0.95);"
        "  border-radius: 15px;"
        "  border: none;"
        "}"
        "QWidget#featureCard:hover {"
        "  background-color: rgba(255, 255, 255, 1.0);"
        "}"
    );

    // Add shadow effect
    auto* shadowEffect = new QGraphicsDropShadowEffect(card);
    shadowEffect->setBlurRadius(15);
    shadowEffect->setColor(QColor(0, 0, 0, 26));
    shadowEffect->setOffset(0, 4);
    card->setGraphicsEffect(shadowEffect);

    iconLabel->setStyleSheet(
        "QLabel#featureIcon {"
        "  font-size: 48pt;"
        "  background: transparent;"
        "}"
    );

    titleLabel->setStyleSheet(
        "QLabel#featureTitle {"
        "  color: #333333;"
        "  font-size: 14pt;"
        "  font-weight: 600;"
        "  background: transparent;"
        "}"
    );

    descLabel->setStyleSheet(
        "QLabel#featureDesc {"
        "  color: #666666;"
        "  font-size: 10pt;"
        "  background: transparent;"
        "}"
    );

    layout->addWidget(iconLabel);
    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);

    return card;
}
