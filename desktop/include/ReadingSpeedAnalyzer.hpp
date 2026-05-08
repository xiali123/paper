#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct SpeedRecord {
    QDate date;
    int wordsRead{0};
    int minutesSpent{0};
    QString paperTitle;
    int paperId{-1};
};

class ReadingSpeedAnalyzer : public QWidget {
    Q_OBJECT

public:
    explicit ReadingSpeedAnalyzer(QWidget* parent = nullptr);

    void addRecord(const SpeedRecord& record);
    QList<SpeedRecord> records() const;
    qreal averageWPM() const;
    qreal averageWPMThisWeek() const;
    int totalWordsRead() const;
    int totalMinutes() const;

signals:
    void recordAdded(const QDate& date, qreal wpm);

private slots:
    void onAdd();
    void onClear();
    void onPeriodChanged(int index);

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawSpeedChart(QPainter& p, const QRect& rect);
    void drawDistribution(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* periodCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<SpeedRecord> records_;
    QSettings settings_;
};
