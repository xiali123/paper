#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BenchEntry {
    int id; QString experiment; QString category; QString status;
    qreal score; int trials; bool passed; QColor color;
};
class PaperLabBench : public QWidget {
    Q_OBJECT
public:
    explicit PaperLabBench(QWidget* parent = nullptr);
    void addEntry(const BenchEntry& entry);
    QList<BenchEntry> entries() const;
    int passedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void benchDone(int id, qreal score);
private slots:
    void onRun();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBenchResults(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BenchEntry> entries_;
    QSettings settings_;
    QPushButton* runBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
