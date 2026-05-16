#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct ReadingMetric {
    int id{-1};
    QString paperTitle;
    QDate date;
    int pagesRead{0};
    int minutesSpent{0};
    qreal comprehension{0};
    QString mode; // "screen", "print", "tablet", "audio"
    QString notes;
    QColor color;
};

class PaperReadingAnalyticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperReadingAnalyticsWidget(QWidget* parent = nullptr);

    void addMetric(const ReadingMetric& metric);
    QList<ReadingMetric> metrics() const;
    QMap<QString, int> modeCounts() const;
    qreal avgComprehension() const;
    int totalPages() const;
    int totalMinutes() const;

signals:
    void metricAdded(int id);
    void analyticsUpdated(int entries);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTrendChart(QPainter& p, const QRect& rect);
    void drawModeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ReadingMetric> metrics_;
    QSettings settings_;
};
