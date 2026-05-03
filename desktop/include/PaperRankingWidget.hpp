#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>

struct RankingEntry {
    int paperId{-1};
    QString title;
    QString authors;
    QString year;
    int rank{0};
    double score{0.0};
    QString metric; // "citations", "rating", "recent", "trending"
    int citations{0};
    double avgRating{0.0};
};

class PaperRankingWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperRankingWidget(QWidget* parent = nullptr);

    void setRankings(const QList<RankingEntry>& entries);
    void setMetric(const QString& metric);

signals:
    void paperClicked(int paperId);
    void metricChanged(const QString& metric);
    void exportRequested(const QList<RankingEntry>& entries);

private slots:
    void onMetricChanged(int index);
    void onItemClicked(QListWidgetItem* item);
    void onExport();
    void onRefresh();

private:
    void setupUI();
    void refreshList();
    QWidget* createRankCard(const RankingEntry& entry, int rank);

    QListWidget* listWidget_{nullptr};
    QComboBox* metricCombo_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<RankingEntry> entries_;
    QString currentMetric_{"citations"};
};
