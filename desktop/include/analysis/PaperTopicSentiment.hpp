#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TopicSentimentEntry {
    int id; QString topic; QString category; QString sentiment;
    qreal score; int mentions; bool positive; QColor color;
};
class PaperTopicSentiment : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicSentiment(QWidget* parent = nullptr);
    void addEntry(const TopicSentimentEntry& entry);
    QList<TopicSentimentEntry> entries() const;
    int positiveCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sentimentAnalyzed(int id, qreal score);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSentimentChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TopicSentimentEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
