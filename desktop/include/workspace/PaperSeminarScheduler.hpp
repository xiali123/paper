#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SeminarEntry {
    int id; QString title; QString category; QString speaker;
    qreal duration; int attendees; bool recorded; QColor color;
};
class PaperSeminarScheduler : public QWidget {
    Q_OBJECT
public:
    explicit PaperSeminarScheduler(QWidget* parent = nullptr);
    void addEntry(const SeminarEntry& entry);
    QList<SeminarEntry> entries() const;
    int recordedCount() const;
    qreal totalDuration() const;
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
    QList<SeminarEntry> entries_;
    QSettings settings_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
