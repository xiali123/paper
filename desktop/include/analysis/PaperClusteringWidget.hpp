#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QList>
#include <QPair>

struct ClusterNode {
    int paperId{-1};
    QString title;
    QString keywords;
    double x{0}, y{0};
    int cluster{-1};
};

class PaperClusteringWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperClusteringWidget(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers); // id, keywords
    void setClusterCount(int k);
    int clusterCount() const { return k_; }

    QList<ClusterNode> clusters() const;
    QMap<int, QList<int>> clusterGroups() const; // cluster -> paperIds

signals:
    void clusteringDone(int k, const QMap<int, QList<int>>& groups);
    void paperClicked(int paperId);
    void clusterClicked(int clusterId);

private slots:
    void onRunClustering();
    void onClusterCountChanged(int value);
    void onItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void runKMeans();
    double similarity(const QString& a, const QString& b) const;
    void refreshClusterList();

    QListWidget* clusterList_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* runBtn_{nullptr};
    QComboBox* methodCombo_{nullptr};

    QList<ClusterNode> nodes_;
    int k_{5};
    QMap<int, QColor> clusterColors_;
};
