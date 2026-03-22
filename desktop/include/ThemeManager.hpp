#pragma once

#include <QObject>
#include <QString>
#include <QColor>
#include <QPalette>
#include <memory>

/**
 * @brief Modern theme manager for PaperCrawler desktop
 *
 * Manages light/dark themes with smooth transitions and
 * gradient backgrounds matching the web frontend
 */
class ThemeManager : public QObject {
    Q_OBJECT

public:
    enum class ThemeMode {
        Light,
        Dark,
        System
    };

    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() = default;

    // Theme management
    void setTheme(ThemeMode mode);
    ThemeMode currentTheme() const { return currentTheme_; }
    void toggleTheme();

    // Apply theme to application
    void applyTheme();

    // Color palette access
    struct Colors {
        // Primary colors
        QColor primaryStart;
        QColor primaryEnd;
        QColor accent;

        // Background colors
        QColor backgroundStart;
        QColor backgroundEnd;
        QColor cardBackground;
        QColor cardHover;

        // Text colors
        QColor textPrimary;
        QColor textSecondary;
        QColor textHint;

        // Border colors
        QColor border;
        QColor borderLight;

        // CCF Level colors
        QColor levelABackground;
        QColor levelAText;
        QColor levelBBackground;
        QColor levelBText;
        QColor levelCBackground;
        QColor levelCText;

        // Status colors
        QColor success;
        QColor warning;
        QColor error;
        QColor info;
    };

    const Colors& colors() const { return colors_; }

    // QSS stylesheet access
    QString stylesheet() const;

signals:
    void themeChanged(ThemeMode mode);
    void colorsChanged(const Colors& colors);

private:
    void setupLightTheme();
    void setupDarkTheme();
    void generateStylesheet();
    QString generateGradientCSS() const;

    ThemeMode currentTheme_;
    Colors colors_;
    QString stylesheet_;
};
