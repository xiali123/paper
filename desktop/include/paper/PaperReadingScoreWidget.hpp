#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QSettings>

struct ReadingScore {
    int paperId{-1};
    QString paperTitle;
    qreal comprehension{0};
    qreal speed{0};
    qreal retention{0};
    qreal engagement{0};
    qreal overall{0};
    QString grade;
    QColor color;
};

class PaperReadingScoreWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperReadingScoreWidget(QWidget* parent = nullptr);

    void addScore(const ReadingScore& score);
    QList<ReadingScore> scores() const;
    qreal averageOverall() const;
    ReadingScore bestScore() const;

signals:
    void scoreUpdated(int count, qreal avg);

private slots:
    void onEvaluate();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawRadarChart(QPainter& p, const QRect& rect);
    void drawScoreBars(QPainter& p, const QRect& rect);
    void drawHistory(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* evalBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ReadingScore> scores_;
    QSettings settings_;
};
