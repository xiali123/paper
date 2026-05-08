#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QTimer>
#include <QList>
#include <QMap>
#include <QElapsedTimer>
#include <QSettings>

struct SessionEntry {
    int id{-1};
    int paperId{-1};
    QString paperTitle;
    QDate date;
    qint64 startTime{0};
    qint64 endTime{0};
    int durationSec{0};
    QString activity;
    QString notes;
    int focusScore{0};
};

class ReadingSessionLog : public QWidget {
    Q_OBJECT

public:
    explicit ReadingSessionLog(QWidget* parent = nullptr);

    void startSession(int paperId, const QString& title);
    void stopSession();
    void addEntry(const SessionEntry& entry);
    void removeEntry(int entryId);
    QList<SessionEntry> entries() const;
    QList<SessionEntry> entriesForDate(const QDate& date) const;
    int totalMinutesThisWeek() const;

signals:
    void sessionStarted(int paperId);
    void sessionStopped(int paperId, int durationSec);
    void entryAdded(int entryId);

private slots:
    void onStart();
    void onStop();
    void onDelete();
    void onFilterChanged(int index);
    void onTimerTick();
    void onExport();

private:
    void setupUI();
    void refreshTree();
    void updateStats();
    void updateTimerDisplay();
    void loadSettings();
    void saveSettings();

    QTreeWidget* sessionTree_{nullptr};
    QLabel* timerLabel_{nullptr};
    QPushButton* startBtn_{nullptr};
    QPushButton* stopBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QComboBox* activityCombo_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QTextEdit* notesEdit_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<SessionEntry> entries_;
    int nextId_{1};
    int currentPaperId_{-1};
    QString currentTitle_;
    QElapsedTimer elapsed_;
    QTimer* tickTimer_{nullptr};
    bool sessionActive_{false};
};
