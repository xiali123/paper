#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct SentimentEntry {
    int id;
    QString text;
    QString sentiment;
    qreal score;
    QString emotion;
    QString section;
    qreal magnitude;
    bool positive;
    QColor color;
};

class PaperSentimentExplorer : public QWidget {
    Q_OBJECT
public:
    explicit PaperSentimentExplorer(QWidget* parent = nullptr);
    void addEntry(const SentimentEntry& entry);
    QList<SentimentEntry> entries() const;
    qreal avgScore() const;
    int positiveCount() const;
    QMap<QString, int> sentimentCounts() const;

signals:
    void sentimentExplored(int id, qreal score);

private slots:
    void onExplore();
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

    QLineEdit* inputField_;
    QPushButton* exploreBtn_;
    QPushButton* clearBtn_;
    QComboBox* sentimentCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<SentimentEntry> entries_;
};
