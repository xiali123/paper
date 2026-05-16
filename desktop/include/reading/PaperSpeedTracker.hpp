#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SpeedTrackerEntry {
    int id; QString paper; QString category; QString method;
    qreal wpm; int pagesRead; bool aboveAverage; QColor color;
};
class PaperSpeedTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperSpeedTracker(QWidget* parent = nullptr);
    void addEntry(const SpeedTrackerEntry& entry);
    QList<SpeedTrackerEntry> entries() const;
    int aboveAverageCount() const;
    qreal avgWpm() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void speedRecorded(int id, qreal wpm);
private slots:
    void onRecord();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSpeedView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SpeedTrackerEntry> entries_;
    QSettings settings_;
    QPushButton* recordBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
