#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QProgressBar>
#include <QList>
#include <QMap>
#include <QSettings>

struct DuplicatePair {
    int id1{-1};
    int id2{-1};
    QString title1;
    QString title2;
    qreal similarity{0.0};
    QString matchType;
};

class PaperDuplicateDetector : public QWidget {
    Q_OBJECT

public:
    explicit PaperDuplicateDetector(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void detect();
    QList<DuplicatePair> duplicates() const;
    QList<DuplicatePair> highConfidence() const;

signals:
    void detectionComplete(int pairCount, int highCount);
    void mergeRequested(int id1, int id2);

private slots:
    void onDetect();
    void onMerge();
    void onIgnore();
    void onFilterChanged(int index);
    void onPairSelected();

private:
    void setupUI();
    void refreshTree();
    void updateStats();
    qreal computeSimilarity(const QString& a, const QString& b);

    QTreeWidget* pairTree_{nullptr};
    QProgressBar* progressBar_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* detectBtn_{nullptr};
    QPushButton* mergeBtn_{nullptr};
    QPushButton* ignoreBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<QPair<int, QString>> papers_;
    QList<DuplicatePair> duplicates_;
    QList<DuplicatePair> ignored_;
    int selectedIdx_{-1};
};
