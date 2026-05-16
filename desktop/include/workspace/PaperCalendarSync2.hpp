#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CalendarSync2Entry {
    int id; QString event; QString category; QString calendar;
    qreal duration; int attendees; bool recurring; QColor color;
};
class PaperCalendarSync2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperCalendarSync2(QWidget* parent = nullptr);
    void addEntry(const CalendarSync2Entry& entry);
    QList<CalendarSync2Entry> entries() const;
    int recurringCount() const;
    qreal avgDuration() const;
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
    void drawSyncView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CalendarSync2Entry> entries_;
    QSettings settings_;
    QPushButton* syncBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
