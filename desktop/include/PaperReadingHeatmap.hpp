#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct HeatmapCell {
    QDate date;
    int count{0};
    qreal intensity{0};
};

class PaperReadingHeatmap : public QWidget {
    Q_OBJECT

public:
    explicit PaperReadingHeatmap(QWidget* parent = nullptr);

    void addReadingDay(const QDate& date, int pagesRead);
    QList<HeatmapCell> cells() const;
    int totalDaysRead() const;
    int longestStreak() const;
    int currentStreak() const;
    int totalPages() const;

signals:
    void dayClicked(const QDate& date, int count);
    void statsUpdated(int days, int streak);

private slots:
    void onYearChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawHeatmap(QPainter& p, const QRect& rect);
    void drawLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void drawMonthChart(QPainter& p, const QRect& rect);
    void updateInfo();
    void rebuildCells();
    void loadSettings();
    void saveSettings();

    QComboBox* yearCombo_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QMap<QDate, int> readingDays_;
    QList<HeatmapCell> cells_;
    int displayYear_{2026};
    QSettings settings_;
};
