#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

struct SpeedEntry {
    int id;
    QString paperTitle;
    int pagesRead;
    int minutesSpent;
    qreal pagesPerHour;
    QString difficulty;
    int comprehension;
    QString session;
    QColor color;
};

class PaperReadingSpeedTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSpeedTracker(QWidget* parent = nullptr);
    void addEntry(const SpeedEntry& entry);
    QList<SpeedEntry> entries() const;
    qreal avgPagesPerHour() const;
    int totalPagesRead() const;
    int totalMinutes() const;

signals:
    void speedRecorded(int id, qreal pagesPerHour);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawSpeedList(QPainter& p, const QRect& rect);
    void drawSpeedChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<SpeedEntry> entries_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
