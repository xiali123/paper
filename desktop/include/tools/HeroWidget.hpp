#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

/**
 * @brief Modern hero section widget matching web frontend
 *
 * Features:
 * - Large title with text shadow
 * - Subtitle with opacity
 * - Health status indicator with pulse animation
 * - Gradient background
 */
class HeroWidget : public QWidget {
    Q_OBJECT

public:
    enum class HealthStatus {
        Healthy,
        Unhealthy,
        Unknown
    };

    explicit HeroWidget(QWidget* parent = nullptr);
    void setHealthStatus(HealthStatus status);

signals:

private:
    void setupUI();
    void setupStyles();
    QString getStatusText() const;
    QString getStatusStyle() const;

    QLabel* titleLabel_{nullptr};
    QLabel* subtitleLabel_{nullptr};
    QWidget* statusWidget_{nullptr};
    QLabel* statusDot_{nullptr};
    QLabel* statusLabel_{nullptr};

    HealthStatus healthStatus_{HealthStatus::Unknown};
};
