#pragma once

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QSettings>
#include <QPainter>
#include <QPaintEvent>
#include <QProgressBar>

struct ReadingSession {
    int id{-1};
    QString paperTitle;
    int durationSec{0};
    int targetSec{1500};
    qint64 timestamp{0};
    bool completed{false};
};

class ReadingTimerWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReadingTimerWidget(QWidget* parent = nullptr);

    void setPaper(const QString& title);
    void startTimer();
    void pauseTimer();
    void resetTimer();
    void setDuration(int minutes);
    QList<ReadingSession> sessions() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    int totalReadingMinutes() const;
    int todayReadingMinutes() const;

signals:
    void timerStarted();
    void timerPaused();
    void timerReset();
    void timerCompleted(int durationSec);
    void sessionSaved(const ReadingSession& session);

private slots:
    void onTick();
    void onStartPause();
    void onReset();
    void onDurationChanged(int index);
    void onClearHistory();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshHistory();
    void updateDisplay();
    void drawTimerFace(QPainter& p);

    QTimer* tickTimer_{nullptr};

    // Timer display
    QLabel* timeLabel_{nullptr};
    QLabel* paperLabel_{nullptr};
    QProgressBar* progressRing_{nullptr};
    QWidget* timerFace_{nullptr};

    // Controls
    QPushButton* startPauseBtn_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QComboBox* durationCombo_{nullptr};
    QPushButton* clearBtn_{nullptr};

    // History
    QListWidget* historyList_{nullptr};
    QLabel* statsLabel_{nullptr};

    // State
    int elapsedSec_{0};
    int targetSec_{1500};
    bool running_{false};
    QString currentPaper_;
    QList<ReadingSession> sessions_;
    int nextSessionId_{1};
};
