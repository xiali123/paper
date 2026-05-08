#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct SentimentResult {
    int paperId{-1};
    QString paperTitle;
    qreal positive{0};
    qreal negative{0};
    qreal neutral{0};
    qreal compound{0}; // -1 to +1
    QString label; // "positive", "negative", "neutral"
    QColor color;
};

class PaperSentimentAnalyzer : public QWidget {
    Q_OBJECT

public:
    explicit PaperSentimentAnalyzer(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void analyze();
    QList<SentimentResult> results() const;
    qreal averageSentiment() const;
    int positiveCount() const;
    int negativeCount() const;

signals:
    void analysisComplete(int count);
    void paperSentiment(int paperId, const QString& sentiment);

private slots:
    void onAnalyze();
    void onSortChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawDistribution(QPainter& p, const QRect& rect);
    void drawSentimentBars(QPainter& p, const QRect& rect);
    void drawDetails(QPainter& p, const QRect& rect);
    void updateInfo();

    QComboBox* sortCombo_{nullptr};
    QPushButton* analyzeBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<SentimentResult> results_;
};
