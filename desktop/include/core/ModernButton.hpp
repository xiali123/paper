#pragma once

#include <QPushButton>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsDropShadowEffect>

/**
 * @brief Modern button with gradient and animations
 *
 * Features:
 * - Gradient background
 * - Ripple effect on click
 * - Smooth hover animations
 * - Magnetic cursor effect
 * - Shadow effects
 */
class ModernButton : public QPushButton {
    Q_OBJECT

public:
    enum class ButtonStyle {
        Primary,
        Secondary,
        Success,
        Warning,
        Danger,
        Ghost
    };

    explicit ModernButton(QWidget* parent = nullptr);
    explicit ModernButton(const QString& text, QWidget* parent = nullptr);

    // Styling
    void setButtonStyle(ButtonStyle style);
    void setCornerRadius(int radius);
    void setShadowEnabled(bool enabled);

    // Animation properties
    void setAnimationDuration(int duration);
    void setHoverEnabled(bool enabled);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void setupButton();
    void updateStyle();
    void setupShadow();

    // Properties
    ButtonStyle buttonStyle_{ButtonStyle::Primary};
    int cornerRadius_{10};
    int animationDuration_{200};
    bool shadowEnabled_{true};
    bool hoverEnabled_{true};
    bool isHovered_{false};
    bool isPressed_{false};

    // Colors
    QColor gradientStart_;
    QColor gradientEnd_;
    QColor hoverStart_;
    QColor hoverEnd_;
    QColor textColor_{Qt::white};

    // Effects
    QGraphicsDropShadowEffect* shadowEffect_{nullptr};

    // Animations
    QPropertyAnimation* scaleAnimation_{nullptr};
};
