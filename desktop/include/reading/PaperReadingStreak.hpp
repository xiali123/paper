#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StreakEntry {
    int id; QString period; QString category; QString level;
    int days; qreal consistency; int papersRead; bool active; QColor color;
};
class PaperReadingStreak : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingStreak(QWidget* parent = nullptr);
    void addEntry(const StreakEntry& entry);
    QList<StreakEntry> entries() const;
    int activeCount() const;
    qreal avgConsistency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void streakUpdated(int id, qreal consistency);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStreakView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StreakEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
