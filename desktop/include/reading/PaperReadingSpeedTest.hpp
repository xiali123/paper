#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct SpeedTestEntry {
    int id;
    QString paperTitle;
    int wordsRead;
    int timeSeconds;
    qreal wpm;
    qreal comprehension;
    QString difficulty;
    QString language;
    QString genre;
    bool passedBaseline;
    QColor color;
};

class PaperReadingSpeedTest : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSpeedTest(QWidget* parent = nullptr);
    void addEntry(const SpeedTestEntry& entry);
    QList<SpeedTestEntry> entries() const;
    qreal avgWPM() const;
    int passedCount() const;
    QMap<QString, int> difficultyCounts() const;

signals:
    void testCompleted(int id, qreal wpm);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onTest();
    void onClear();
    void drawResultList(QPainter& p, const QRect& rect);
    void drawDifficultyChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* difficultyCombo_;
    QPushButton* testBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<SpeedTestEntry> entries_;
    QSettings settings_;
};
