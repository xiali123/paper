#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TimerEntry {
    int id; QString task; QString category; QString mode;
    int duration; qreal efficiency; QString date; bool completed; QColor color;
};
class PaperTimerWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaperTimerWidget(QWidget* parent = nullptr);
    void addEntry(const TimerEntry& entry);
    QList<TimerEntry> entries() const;
    int completedCount() const;
    qreal avgEfficiency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void timerLogged(int id, qreal efficiency);
private slots:
    void onLog();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTimerList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TimerEntry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
