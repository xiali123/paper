#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SpeedTestEntry {
    int id; QString paper; QString category; QString difficulty;
    qreal wpm; int comprehension; bool aboveAvg; QColor color;
};
class PaperReadingSpeedTest2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSpeedTest2(QWidget* parent = nullptr);
    void addEntry(const SpeedTestEntry& entry);
    QList<SpeedTestEntry> entries() const;
    int aboveAvgCount() const;
    qreal avgWpm() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void testComplete(int id, qreal wpm);
private slots:
    void onTest();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSpeedChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SpeedTestEntry> entries_;
    QSettings settings_;
    QPushButton* testBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
