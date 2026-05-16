#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

/**
 * @brief Modern card widget with glass morphism effect
 *
 * Provides a beautiful card component with:
 * - Rounded corners
 * - Drop shadow
 * - Hover animations
 * - Glass morphism effect
 */
class ModernCard : public QWidget {
    Q_OBJECT

public:
    explicit ModernCard(QWidget* parent = nullptr);
    explicit ModernCard(const QString& title, QWidget* parent = nullptr);

    // Card properties
    void setTitle(const QString& title);
    void setContent(QWidget* content);
    void setCornerRadius(int radius);
    void setShadowEnabled(bool enabled);
    void setHoverEnabled(bool enabled);

    // Styling
    void setCardColor(const QColor& color);
    void setBorderColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void setupUI();
    void setupShadow();
    void setupAnimations();

    // UI Components
    QVBoxLayout* layout_{nullptr};
    QLabel* titleLabel_{nullptr};
    QWidget* contentWidget_{nullptr};

    // Properties
    int cornerRadius_{15};
    bool shadowEnabled_{true};
    bool hoverEnabled_{true};
    bool isHovered_{false};

    // Colors
    QColor cardColor_{255, 255, 255, 245};
    QColor borderColor_{229, 231, 235};

    // Effects
    QGraphicsDropShadowEffect* shadowEffect_{nullptr};

    // Animations
    QPropertyAnimation* hoverAnimation_{nullptr};
};
