#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ShortcutEntry {
    int id;
    QString action;
    QString shortcut;
    QString category;
    QString context;
    int usageCount;
    bool custom;
    bool enabled;
    QColor color;
};

class PaperKeyboardShortcut : public QWidget {
    Q_OBJECT
public:
    explicit PaperKeyboardShortcut(QWidget* parent = nullptr);
    void addEntry(const ShortcutEntry& entry);
    QList<ShortcutEntry> entries() const;
    int enabledCount() const;
    int totalUsage() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void shortcutAssigned(int id, QString shortcut);

private slots:
    void onAssign();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawShortcutList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* assignBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ShortcutEntry> entries_;
};
