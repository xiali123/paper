#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QQueue>
#include <QSettings>

struct DownloadTask {
    int id{-1};
    QString title;
    QString url;
    QString savePath;
    QString status{"pending"};
    int progress{0};
    qint64 fileSize{0};
    QString error;
};

class BatchDownloadWidget : public QWidget {
    Q_OBJECT

public:
    explicit BatchDownloadWidget(QWidget* parent = nullptr);

    void addTask(const QString& title, const QString& url, const QString& savePath = "");
    void addTasks(const QList<QPair<QString, QString>>& tasks);
    void removeTask(int taskId);
    void clearCompleted();
    void startAll();
    void pauseAll();
    int activeCount() const;
    int completedCount() const;

signals:
    void downloadStarted(int taskId);
    void downloadProgress(int taskId, int percent);
    void downloadCompleted(int taskId, const QString& filePath);
    void downloadFailed(int taskId, const QString& error);
    void allCompleted(int total, int success, int failed);
    void queueChanged(int pending, int active, int completed);

private slots:
    void onStart();
    void onPause();
    void onRemoveSelected();
    void onClearCompleted();
    void onConcurrentChanged(int index);
    void tick();

private:
    void setupUI();
    void processQueue();
    void updateStats();
    void refreshTable();
    void loadSettings();
    void saveSettings();

    QTableWidget* table_{nullptr};
    QProgressBar* totalProgress_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* startBtn_{nullptr};
    QPushButton* pauseBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QComboBox* concurrentCombo_{nullptr};

    QMap<int, DownloadTask> tasks_;
    QQueue<int> pendingQueue_;
    QSet<int> activeSet_;
    int maxConcurrent_{3};
    int nextTaskId_{1};
    QTimer* tickTimer_{nullptr};
};
