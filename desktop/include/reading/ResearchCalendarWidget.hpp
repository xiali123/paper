#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct CalendarEvent {
    int id{-1};
    QString title;
    QString category; // "reading", "deadline", "meeting", "review", "submission"
    QDate date;
    QColor color;
    QString notes;
};

class ResearchCalendarWidget : public QWidget {
    Q_OBJECT

public:
    explicit ResearchCalendarWidget(QWidget* parent = nullptr);

    void addEvent(const CalendarEvent& event);
    QList<CalendarEvent> events() const;
    QList<CalendarEvent> eventsForDate(const QDate& date) const;
    QMap<QString, int> categoryCounts() const;

signals:
    void eventClicked(int eventId);
    void calendarUpdated(int count);

private slots:
    void onAdd();
    void onMonthChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawCalendar(QPainter& p, const QRect& rect);
    void drawUpcomingList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* monthCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<CalendarEvent> events_;
    int displayMonth_{5};
    int displayYear_{2026};
    QSettings settings_;
};
