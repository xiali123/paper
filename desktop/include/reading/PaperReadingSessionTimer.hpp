#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct SessionTimerEntry {
    int id;
    QString paperTitle;
    int durationMin;
    qreal focusScore;
    QString activity;
    int pagesRead;
    qreal comprehension;
    QString device;
    QString mood;
    QColor color;
};

class PaperReadingSessionTimer : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSessionTimer(QWidget* parent = nullptr);
    void addEntry(const SessionTimerEntry& entry);
    QList<SessionTimerEntry> entries() const;
    int totalMinutes() const;
    qreal avgFocus() const;
    QMap<QString, int> activityCounts() const;

signals:
    void sessionLogged(int id, int duration);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onLog();
    void onClear();
    void drawSessionList(QPainter& p, const QRect& rect);
    void drawActivityChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* activityCombo_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<SessionTimerEntry> entries_;
    QSettings settings_;
};
