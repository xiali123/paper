#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct MeetingEntry {
    int id;
    QString title;
    QString organizer;
    QString date;
    int duration;
    int attendees;
    QString type;
    QString status;
    bool recurring;
    QColor color;
};

class PaperMeetingScheduler : public QWidget {
    Q_OBJECT
public:
    explicit PaperMeetingScheduler(QWidget* parent = nullptr);
    void addEntry(const MeetingEntry& entry);
    QList<MeetingEntry> entries() const;
    int totalMinutes() const;
    int recurringCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void meetingScheduled(int id, int duration);

private slots:
    void onSchedule();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawMeetingList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* typeCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<MeetingEntry> entries_;
};
