#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct SentimentEntry {
    int id;
    QString text;
    QString sentiment;
    qreal score;
    QString section;
    int wordCount;
    QString topic;
    qreal confidence;
    QString category;
    bool positive;
    QColor color;
};

class PaperSentimentTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperSentimentTracker(QWidget* parent = nullptr);
    void addEntry(const SentimentEntry& entry);
    QList<SentimentEntry> entries() const;
    qreal avgScore() const;
    int positiveCount() const;
    QMap<QString, int> sentimentCounts() const;

signals:
    void sentimentTracked(int id, qreal score);

private slots:
    void onTrack();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawSentimentList(QPainter& p, const QRect& rect);
    void drawSentimentChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<SentimentEntry> entries_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
