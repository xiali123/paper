#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QList>

struct DepNode {
    int paperId{-1};
    QString title;
    int depth{0};
    QList<int> dependsOn;
    QList<int> requiredBy;
};

class PaperDependencyWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperDependencyWidget(QWidget* parent = nullptr);

    void addPaper(int paperId, const QString& title);
    void addDependency(int paperId, int dependsOnId);
    void removeDependency(int paperId, int dependsOnId);
    void buildFromPairs(const QList<QPair<int, int>>& deps);
    void clear();
    QList<DepNode> topologicalSort() const;
    QList<int> getChain(int paperId) const;
    int dependencyDepth(int paperId) const;

signals:
    void dependencyAdded(int paperId, int dependsOnId);
    void chainSelected(int paperId, const QList<int>& chain);
    void cycleDetected(const QList<int>& cycle);

private slots:
    void onAddDep();
    void onRemoveDep();
    void onCheckCycles();
    void onTopSort();
    void onNodeClicked(QTreeWidgetItem* item, int column);
    void onDepClicked(int row, int col);

private:
    void setupUI();
    void refreshTree();
    void refreshTable();
    bool hasCycle() const;
    bool dfs(int node, QSet<int>& visited, QSet<int>& recursion, QList<int>& cycle) const;

    QTreeWidget* depTree_{nullptr};
    QTableWidget* depTable_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* checkCycleBtn_{nullptr};
    QPushButton* topSortBtn_{nullptr};
    QComboBox* sourceCombo_{nullptr};
    QComboBox* targetCombo_{nullptr};

    QMap<int, DepNode> nodes_;
};
