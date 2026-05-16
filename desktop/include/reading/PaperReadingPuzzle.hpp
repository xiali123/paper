#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct PuzzleEntry {
    int id;
    QString puzzleName;
    QString difficulty;
    int questions;
    int correct;
    qreal score;
    QString topic;
    int timeSeconds;
    QString badge;
    bool passed;
    QColor color;
};

class PaperReadingPuzzle : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingPuzzle(QWidget* parent = nullptr);
    void addEntry(const PuzzleEntry& entry);
    QList<PuzzleEntry> entries() const;
    int passedCount() const;
    qreal avgScore() const;
    QMap<QString, int> topicCounts() const;
signals:
    void puzzleSolved(int id, qreal score);
private slots:
    void onSolve();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawPuzzleList(QPainter& p, const QRect& rect);
    void drawTopicChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PuzzleEntry> entries_;
    QSettings settings_;
    QPushButton* solveBtn_;
    QPushButton* clearBtn_;
    QComboBox* topicCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
