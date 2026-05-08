#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct RatingEntry {
    int paperId{-1};
    QString title;
    qreal novelty{0};
    qreal methodology{0};
    qreal clarity{0};
    qreal significance{0};
    qreal reproducibility{0};
    qreal overall{0};
    QString notes;
};

class PaperRatingChart : public QWidget {
    Q_OBJECT

public:
    explicit PaperRatingChart(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void addRating(const RatingEntry& rating);
    void removeRating(int paperId);
    QList<RatingEntry> ratings() const;
    RatingEntry ratingForPaper(int paperId) const;

signals:
    void ratingAdded(int paperId, qreal overall);
    void ratingUpdated(int paperId, qreal overall);
    void paperSelected(int paperId);

private slots:
    void onChartTypeChanged(int index);
    void onSortChanged(int index);
    void onAddSample();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void drawRadarChart(QPainter& p, const QRect& rect, const RatingEntry& r);
    void drawBarChart(QPainter& p, const QRect& rect);
    void drawScatterPlot(QPainter& p, const QRect& rect);
    void updateStats();

    QComboBox* chartTypeCombo_{nullptr};
    QComboBox* sortCombo_{nullptr};
    QPushButton* sampleBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<RatingEntry> ratings_;
    QSettings settings_;
};
