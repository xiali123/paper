#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BatchTestEntry {
    int id; QString suite; QString category; QString status;
    qreal passRate; int total; int failed; bool passed; QColor color;
};
class PaperBatchTester : public QWidget {
    Q_OBJECT
public:
    explicit PaperBatchTester(QWidget* parent = nullptr);
    void addEntry(const BatchTestEntry& entry);
    QList<BatchTestEntry> entries() const;
    int passedCount() const;
    qreal avgPassRate() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void suiteComplete(int id, qreal passRate);
private slots:
    void onRun();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTestList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BatchTestEntry> entries_;
    QSettings settings_;
    QPushButton* runBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
