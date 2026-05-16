#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CalendarEntry {
    int id; QString event; QString category; QString date;
    qreal duration; int attendees; bool allDay; QColor color;
};
class PaperTeamCalendar : public QWidget {
    Q_OBJECT
public:
    explicit PaperTeamCalendar(QWidget* parent = nullptr);
    void addEntry(const CalendarEntry& entry);
    QList<CalendarEntry> entries() const;
    int allDayCount() const;
    qreal avgDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void eventScheduled(int id, qreal duration);
private slots:
    void onSchedule();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCalendarView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CalendarEntry> entries_;
    QSettings settings_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
