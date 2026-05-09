#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QTimer>

struct PomodoroSession {
    int id{-1};
    QString task;
    int durationMinutes{25};
    int elapsedSeconds{0};
    bool completed{false};
    QString category; // "reading", "writing", "review", "analysis"
    QColor color;
};

class PaperPomodoroTimer : public QWidget {
    Q_OBJECT

public:
    explicit PaperPomodoroTimer(QWidget* parent = nullptr);

    void addSession(const PomodoroSession& session);
    QList<PomodoroSession> sessions() const;
    QMap<QString, int> categoryCounts() const;
    int totalMinutes() const;
    int completedSessions() const;

signals:
    void sessionCompleted(int id);
    void timerTick(int remaining);

private slots:
    void onStart();
    void onStop();
    void onTick();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTimerDial(QPainter& p, const QRect& rect);
    void drawSessionList(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* startBtn_{nullptr};
    QPushButton* stopBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};
    QTimer* timer_{nullptr};

    QList<PomodoroSession> sessions_;
    int currentSession_{-1};
    int currentElapsed_{0};
    QSettings settings_;
};
