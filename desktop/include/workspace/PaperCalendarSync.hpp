#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CalendarSyncEntry {
    int id; QString event; QString category; QString calendar;
    qreal duration; int attendees; bool synced; QColor color;
};
class PaperCalendarSync : public QWidget {
    Q_OBJECT
public:
    explicit PaperCalendarSync(QWidget* parent = nullptr);
    void addEntry(const CalendarSyncEntry& entry);
    QList<CalendarSyncEntry> entries() const;
    int syncedCount() const;
    qreal totalDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void eventSynced(int id, qreal duration);
private slots:
    void onSync();
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
    QList<CalendarSyncEntry> entries_;
    QSettings settings_;
    QPushButton* syncBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
