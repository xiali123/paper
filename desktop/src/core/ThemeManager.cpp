#include "core/ThemeManager.hpp"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent), currentTheme_(ThemeMode::Light) {
    setupLightTheme();
    generateStylesheet();
}

void ThemeManager::setTheme(ThemeMode mode) {
    if (mode == ThemeMode::System) {
        // Detect system theme (simplified for Windows)
        mode = ThemeMode::Light;
    }

    if (currentTheme_ != mode) {
        currentTheme_ = mode;

        if (mode == ThemeMode::Light) {
            setupLightTheme();
        } else {
            setupDarkTheme();
        }

        generateStylesheet();
        applyTheme();
        emit themeChanged(mode);
        emit colorsChanged(colors_);
    }
}

void ThemeManager::toggleTheme() {
    setTheme(currentTheme_ == ThemeMode::Light
             ? ThemeMode::Dark
             : ThemeMode::Light);
}

void ThemeManager::applyTheme() {
    qApp->setStyleSheet(stylesheet());
}

void ThemeManager::setupLightTheme() {
    // Primary gradient - matching web frontend
    colors_.primaryStart = QColor(102, 126, 234);      // #667eea
    colors_.primaryEnd = QColor(118, 75, 162);          // #764ba2
    colors_.accent = QColor(99, 102, 241);              // #6366f1

    // Background gradient
    colors_.backgroundStart = QColor(243, 244, 246);    // #f3f4f6
    colors_.backgroundEnd = QColor(229, 231, 235);      // #e5e7eb

    // Card colors with glass effect
    colors_.cardBackground = QColor(255, 255, 255, 245); // rgba(255, 255, 255, 0.95)
    colors_.cardHover = QColor(255, 255, 255, 255);

    // Text colors
    colors_.textPrimary = QColor(17, 24, 39);            // #111827
    colors_.textSecondary = QColor(75, 85, 99);          // #4b5563
    colors_.textHint = QColor(156, 163, 175);            // #9ca3af

    // Border colors
    colors_.border = QColor(229, 231, 235);              // #e5e7eb
    colors_.borderLight = QColor(243, 244, 246);         // #f3f4f6

    // CCF Level colors - matching web frontend
    colors_.levelABackground = QColor(254, 202, 202);    // #fecaca
    colors_.levelAText = QColor(153, 27, 27);            // #991b1b
    colors_.levelBBackground = QColor(254, 215, 170);    // #fed7aa
    colors_.levelBText = QColor(154, 52, 18);            // #9a3412
    colors_.levelCBackground = QColor(209, 213, 219);    // #d1d5db
    colors_.levelCText = QColor(55, 65, 81);             // #374151

    // Status colors
    colors_.success = QColor(34, 197, 94);               // #22c55e
    colors_.warning = QColor(251, 146, 60);              // #fb923c
    colors_.error = QColor(239, 68, 68);                 // #ef4444
    colors_.info = QColor(59, 130, 246);                 // #3b82f6
}

void ThemeManager::setupDarkTheme() {
    // Primary gradient - adjusted for dark mode
    colors_.primaryStart = QColor(124, 58, 237);         // #7c3aed
    colors_.primaryEnd = QColor(147, 51, 234);           // #9333ea
    colors_.accent = QColor(139, 92, 246);               // #8b5cf6

    // Background gradient
    colors_.backgroundStart = QColor(17, 24, 39);        // #111827
    colors_.backgroundEnd = QColor(31, 41, 55);          // #1f2937

    // Card colors with glass effect
    colors_.cardBackground = QColor(31, 41, 55, 245);    // rgba(31, 41, 55, 0.95)
    colors_.cardHover = QColor(55, 65, 81, 255);

    // Text colors
    colors_.textPrimary = QColor(243, 244, 246);         // #f3f4f6
    colors_.textSecondary = QColor(209, 213, 219);       // #d1d5db
    colors_.textHint = QColor(156, 163, 175);           // #9ca3af

    // Border colors
    colors_.border = QColor(55, 65, 81);                 // #374151
    colors_.borderLight = QColor(75, 85, 99);            // #4b5563

    // CCF Level colors - adjusted for dark mode
    colors_.levelABackground = QColor(185, 28, 28);      // #b91c1c
    colors_.levelAText = QColor(254, 226, 226);          // #fee2e2
    colors_.levelBBackground = QColor(180, 83, 9);       // #b45309
    colors_.levelBText = QColor(255, 237, 213);          // #ffedcc
    colors_.levelCBackground = QColor(75, 85, 99);       // #4b5563
    colors_.levelCText = QColor(243, 244, 246);          // #f3f4f6

    // Status colors
    colors_.success = QColor(74, 222, 128);              // #4ade80
    colors_.warning = QColor(251, 191, 36);              // #fbbf24
    colors_.error = QColor(248, 113, 113);               // #f87171
    colors_.info = QColor(96, 165, 250);                 // #60a5fa
}

QString ThemeManager::generateGradientCSS() const {
    return QString(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "stop:0 rgba(%1, %2, %3, %4), "
        "stop:1 rgba(%5, %6, %7, %8));"
    ).arg(colors_.backgroundStart.red())
     .arg(colors_.backgroundStart.green())
     .arg(colors_.backgroundStart.blue())
     .arg(colors_.backgroundStart.alpha())
     .arg(colors_.backgroundEnd.red())
     .arg(colors_.backgroundEnd.green())
     .arg(colors_.backgroundEnd.blue())
     .arg(colors_.backgroundEnd.alpha());
}

void ThemeManager::generateStylesheet() {
    QString css;

    // Main Window
    css += "QMainWindow {"
           "  " + generateGradientCSS() +
           "  border: none;"
           "}\n";

    // Central Widget
    css += "QWidget {"
           "  background-color: transparent;"
           "  color: #" + colors_.textPrimary.name() + ";"
           "  font-family: 'Segoe UI', 'Microsoft YaHei UI', sans-serif;"
           "  font-size: 10pt;"
           "}\n";

    // Modern Card Widget
    css += ".ModernCard {"
           "  background-color: rgba(" +
           QString::number(colors_.cardBackground.red()) + ", " +
           QString::number(colors_.cardBackground.green()) + ", " +
           QString::number(colors_.cardBackground.blue()) + ", " +
           QString::number(colors_.cardBackground.alpha() / 255.0) + ");"
           "  border-radius: 15px;"
           "  border: 1px solid #" + colors_.border.name() + ";"
           "  padding: 8px;"
           "  margin: 4px;"
           "}\n";

    // Push Button - Modern Style
    css += "QPushButton {"
           "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
           "    stop:0 #" + colors_.primaryStart.name() + ", "
           "    stop:1 #" + colors_.primaryEnd.name() + ");"
           "  color: white;"
           "  border-radius: 10px;"
           "  border: none;"
           "  padding: 10px 24px;"
           "  font-weight: 600;"
           "  font-size: 11pt;"
           "}\n";

    css += "QPushButton:hover {"
           "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
           "    stop:0 #" + colors_.primaryEnd.name() + ", "
           "    stop:1 #" + colors_.primaryStart.name() + ");"
           "  transform: translateY(-2px);"
           "}\n";

    css += "QPushButton:pressed {"
           "  transform: translateY(0px);"
           "  padding: 8px 22px;"
           "}\n";

    // Line Edit - Search Box
    css += "QLineEdit {"
           "  background-color: #" + colors_.cardBackground.name() + ";"
           "  border: 2px solid #" + colors_.border.name() + ";"
           "  border-radius: 12px;"
           "  padding: 12px 16px;"
           "  font-size: 11pt;"
           "  color: #" + colors_.textPrimary.name() + ";"
           "}\n";

    css += "QLineEdit:focus {"
           "  border: 2px solid #" + colors_.accent.name() + ";"
           "  background-color: #" + colors_.cardHover.name() + ";"
           "}\n";

    // Table View - Results
    css += "QTableView {"
           "  background-color: #" + colors_.cardBackground.name() + ";"
           "  alternate-background-color: #" + colors_.borderLight.name() + ";"
           "  gridline-color: #" + colors_.border.name() + ";"
           "  border: none;"
           "  border-radius: 15px;"
           "  selection-background-color: rgba(" +
           QString::number(colors_.accent.red()) + ", " +
           QString::number(colors_.accent.green()) + ", " +
           QString::number(colors_.accent.blue()) + ", 0.3);"
           "  selection-color: #" + colors_.textPrimary.name() + ";"
           "}\n";

    css += "QTableView::item {"
           "  border: none;"
           "  padding: 8px;"
           "  border-bottom: 1px solid #" + colors_.border.name() + ";"
           "}\n";

    css += "QTableView::item:hover {"
           "  background-color: #" + colors_.cardHover.name() + ";"
           "}\n";

    css += "QTableView::item:selected {"
           "  background-color: rgba(" +
           QString::number(colors_.accent.red()) + ", " +
           QString::number(colors_.accent.green()) + ", " +
           QString::number(colors_.accent.blue()) + ", 0.2);"
           "}\n";

    // Header View
    css += "QHeaderView::section {"
           "  background-color: #" + colors_.borderLight.name() + ";"
           "  color: #" + colors_.textSecondary.name() + ";"
           "  padding: 12px;"
           "  border: none;"
           "  border-bottom: 2px solid #" + colors_.border.name() + ";"
           "  font-weight: 600;"
           "}\n";

    // Scroll Bar
    css += "QScrollBar:vertical {"
           "  background-color: #" + colors_.borderLight.name() + ";"
           "  width: 12px;"
           "  border-radius: 6px;"
           "  margin: 0px;"
           "}\n";

    css += "QScrollBar::handle:vertical {"
           "  background-color: #" + colors_.border.name() + ";"
           "  border-radius: 6px;"
           "  min-height: 30px;"
           "}\n";

    css += "QScrollBar::handle:vertical:hover {"
           "  background-color: #" + colors_.textHint.name() + ";"
           "}\n";

    css += "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
           "  height: 0px;"
           "}\n";

    // Label
    css += "QLabel {"
           "  color: #" + colors_.textSecondary.name() + ";"
           "  background: transparent;"
           "}\n";

    // Progress Bar
    css += "QProgressBar {"
           "  background-color: #" + colors_.borderLight.name() + ";"
           "  border: none;"
           "  border-radius: 8px;"
           "  text-align: center;"
           "  color: #" + colors_.textPrimary.name() + ";"
           "}\n";

    css += "QProgressBar::chunk {"
           "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
           "    stop:0 #" + colors_.primaryStart.name() + ", "
           "    stop:1 #" + colors_.primaryEnd.name() + ");"
           "  border-radius: 8px;"
           "}\n";

    // Menu Bar
    css += "QMenuBar {"
           "  background-color: #" + colors_.cardBackground.name() + ";"
           "  border: none;"
           "  color: #" + colors_.textPrimary.name() + ";"
           "}\n";

    css += "QMenuBar::item {"
           "  padding: 8px 16px;"
           "  border-radius: 6px;"
           "}\n";

    css += "QMenuBar::item:selected {"
           "  background-color: rgba(" +
           QString::number(colors_.accent.red()) + ", " +
           QString::number(colors_.accent.green()) + ", " +
           QString::number(colors_.accent.blue()) + ", 0.2);"
           "}\n";

    // Menu
    css += "QMenu {"
           "  background-color: #" + colors_.cardBackground.name() + ";"
           "  border: 1px solid #" + colors_.border.name() + ";"
           "  border-radius: 10px;"
           "  padding: 8px;"
           "}\n";

    css += "QMenu::item {"
           "  padding: 8px 24px;"
           "  border-radius: 6px;"
           "}\n";

    css += "QMenu::item:selected {"
           "  background-color: rgba(" +
           QString::number(colors_.accent.red()) + ", " +
           QString::number(colors_.accent.green()) + ", " +
           QString::number(colors_.accent.blue()) + ", 0.2);"
           "}\n";

    // Status Bar
    css += "QStatusBar {"
           "  background-color: #" + colors_.cardBackground.name() + ";"
           "  color: #" + colors_.textSecondary.name() + ";"
           "  border: none;"
           "  border-top: 1px solid #" + colors_.border.name() + ";"
           "}\n";

    // Tool Bar
    css += "QToolBar {"
           "  background-color: #" + colors_.cardBackground.name() + ";"
           "  border: none;"
           "  spacing: 8px;"
           "  padding: 8px;"
           "}\n";

    stylesheet_ = css;
}

QString ThemeManager::stylesheet() const {
    return stylesheet_;
}
