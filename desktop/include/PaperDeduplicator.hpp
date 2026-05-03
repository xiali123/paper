#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QPair>

struct Paper;

class PaperDeduplicator : public QWidget {
    Q_OBJECT

public:
    explicit PaperDeduplicator(QWidget* parent = nullptr);

    void setPapers(const QList<Paper>& papers);
    QList<QPair<int, int>> findDuplicates() const;

signals:
    void mergeRequested(int keepId, int removeId);
    void papersMerged(int count);

private slots:
    void onScan();
    void onMergeSelected();
    void onSkip();
    void onMergeAll();

private:
    void setupUI();
    void showNextPair();

    QTableWidget* leftTable_{nullptr};
    QTableWidget* rightTable_{nullptr};
    QLabel* statusLabel_{nullptr};
    QLabel* progressLabel_{nullptr};
    QPushButton* scanBtn_{nullptr};
    QPushButton* mergeBtn_{nullptr};
    QPushButton* skipBtn_{nullptr};
    QPushButton* mergeAllBtn_{nullptr};

    QList<Paper> papers_;
    QList<QPair<int, int>> duplicatePairs_;
    int currentPair_{0};
};
