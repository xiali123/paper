#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SeminarScheduler2Entry {
    int id; QString seminar; QString category; QString speaker;
    qreal duration; int attendees; bool recorded; QColor color;
};
class PaperSeminarScheduler2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperSeminarScheduler2(QWidget* parent = nullptr);
    void addEntry(const SeminarScheduler2Entry& entry);
    QList<SeminarScheduler2Entry> entries() const;
    int recordedCount() const;
    qreal avgDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void seminarScheduled(int id, qreal duration);
private slots:
    void onSchedule();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScheduleView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SeminarScheduler2Entry> entries_;
    QSettings settings_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
