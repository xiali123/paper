#pragma once

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QJsonObject>

struct ReadingProgress {
    int paperId{-1};
    QString paperTitle;
    int totalPages{0};
    int currentPage{0};
    int percent{0};
    qint64 lastRead{0};
    QString status{"unread"}; // unread, reading, read
};

class ProgressTracker : public QWidget {
    Q_OBJECT

public:
    explicit ProgressTracker(QWidget* parent = nullptr);

    void setProgress(int paperId, const QString& title, int current, int total);
    ReadingProgress getProgress(int paperId) const;
    QList<ReadingProgress> allProgress() const;

    QMap<QString, int> statistics() const;

    void loadFromSettings();
    void saveToSettings();

signals:
    void progressUpdated(int paperId, int percent);
    void paperCompleted(int paperId);
    void statsChanged(const QMap<QString, int>& stats);

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onResetProgress();

private:
    void setupUI();
    void refreshList();
    void updateStats();
    QWidget* createProgressCard(const ReadingProgress& rp);

    QListWidget* listWidget_{nullptr};
    QLabel* statsLabel_{nullptr};
    QMap<int, ReadingProgress> progressMap_;
};
