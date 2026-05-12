#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Sentiment2Entry {
    int id; QString topic; QString category; QString emotion;
    qreal intensity; int mentions; bool positive; QColor color;
};
class PaperTopicSentiment2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicSentiment2(QWidget* parent = nullptr);
    void addEntry(const Sentiment2Entry& entry);
    QList<Sentiment2Entry> entries() const;
    int positiveCount() const;
    qreal avgIntensity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sentimentMapped(int id, qreal intensity);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSentimentMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Sentiment2Entry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
