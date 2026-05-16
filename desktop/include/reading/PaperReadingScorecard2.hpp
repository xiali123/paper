#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingScorecard2Entry {
    int id; QString paper; QString category; QString metric;
    qreal score; int attempts; bool mastered; QColor color;
};
class PaperReadingScorecard2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingScorecard2(QWidget* parent = nullptr);
    void addEntry(const ReadingScorecard2Entry& entry);
    QList<ReadingScorecard2Entry> entries() const;
    int masteredCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void scoreUpdated(int id, qreal score);
private slots:
    void onScore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScorecardView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingScorecard2Entry> entries_;
    QSettings settings_;
    QPushButton* scoreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
