#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct TimeEntry {
    int id;
    QString taskName;
    QString project;
    QString category;
    qreal hours;
    qreal billable;
    QString date;
    bool overtime;
    QColor color;
};

class PaperTimeTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperTimeTracker(QWidget* parent = nullptr);
    void addEntry(const TimeEntry& entry);
    QList<TimeEntry> entries() const;
    qreal totalHours() const;
    qreal totalBillable() const;
    QMap<QString, int> projectCounts() const;

signals:
    void timeTracked(int id, qreal hours);

private slots:
    void onTrack();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawTimeList(QPainter& p, const QRect& rect);
    void drawProjectChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* projectCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<TimeEntry> entries_;
};
