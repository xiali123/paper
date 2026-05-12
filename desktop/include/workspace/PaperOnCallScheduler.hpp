#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct OnCallEntry {
    int id; QString member; QString category; QString shift;
    QString date; qreal load; int incidents; bool primary; QColor color;
};
class PaperOnCallScheduler : public QWidget {
    Q_OBJECT
public:
    explicit PaperOnCallScheduler(QWidget* parent = nullptr);
    void addEntry(const OnCallEntry& entry);
    QList<OnCallEntry> entries() const;
    int primaryCount() const;
    qreal avgLoad() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void shiftScheduled(int id, qreal load);
private slots:
    void onSchedule();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScheduleList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<OnCallEntry> entries_;
    QSettings settings_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
