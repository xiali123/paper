#include "visualization/PaperDependencyWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QInputDialog>
#include <QQueue>

PaperDependencyWidget::PaperDependencyWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperDependencyWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Add dependency row
    auto* addRow = new QHBoxLayout();
    addRow->addWidget(new QLabel("From:"));
    sourceCombo_ = new QComboBox();
    sourceCombo_->setMinimumWidth(150);
    addRow->addWidget(sourceCombo_, 1);
    addRow->addWidget(new QLabel("depends on:"));
    targetCombo_ = new QComboBox();
    targetCombo_->setMinimumWidth(150);
    addRow->addWidget(targetCombo_, 1);

    addBtn_ = new QPushButton("Add Dep");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperDependencyWidget::onAddDep);
    addRow->addWidget(addBtn_);

    removeBtn_ = new QPushButton("Remove Dep");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &PaperDependencyWidget::onRemoveDep);
    addRow->addWidget(removeBtn_);

    layout->addLayout(addRow);

    // Action buttons
    auto* actionRow = new QHBoxLayout();
    checkCycleBtn_ = new QPushButton("Check Cycles");
    connect(checkCycleBtn_, &QPushButton::clicked, this, &PaperDependencyWidget::onCheckCycles);
    actionRow->addWidget(checkCycleBtn_);

    topSortBtn_ = new QPushButton("Topological Sort");
    topSortBtn_->setStyleSheet("QPushButton { background: #8b5cf6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(topSortBtn_, &QPushButton::clicked, this, &PaperDependencyWidget::onTopSort);
    actionRow->addWidget(topSortBtn_);

    actionRow->addStretch();
    layout->addLayout(actionRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Dependency tree
    depTree_ = new QTreeWidget();
    depTree_->setHeaderLabels({"Paper", "Depth", "Deps"});
    depTree_->setColumnWidth(0, 200);
    depTree_->setColumnWidth(1, 50);
    depTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(depTree_, &QTreeWidget::itemClicked, this, &PaperDependencyWidget::onNodeClicked);
    splitter->addWidget(depTree_);

    // Dependency table
    depTable_ = new QTableWidget();
    depTable_->setColumnCount(3);
    depTable_->setHorizontalHeaderLabels({"Paper", "Depends On", "Required By"});
    depTable_->horizontalHeader()->setStretchLastSection(true);
    depTable_->setColumnWidth(0, 160);
    depTable_->setColumnWidth(1, 160);
    depTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    depTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(depTable_, &QTableWidget::cellClicked, this, &PaperDependencyWidget::onDepClicked);
    splitter->addWidget(depTable_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("Add papers and dependencies");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperDependencyWidget::addPaper(int paperId, const QString& title) {
    if (nodes_.contains(paperId)) return;
    DepNode node;
    node.paperId = paperId;
    node.title = title;
    nodes_[paperId] = node;
    sourceCombo_->addItem(title, paperId);
    targetCombo_->addItem(title, paperId);
    refreshTable();
    refreshTree();
}

void PaperDependencyWidget::addDependency(int paperId, int dependsOnId) {
    if (!nodes_.contains(paperId) || !nodes_.contains(dependsOnId)) return;
    if (nodes_[paperId].dependsOn.contains(dependsOnId)) return;
    nodes_[paperId].dependsOn.append(dependsOnId);
    nodes_[dependsOnId].requiredBy.append(paperId);
    refreshTable();
    refreshTree();
    emit dependencyAdded(paperId, dependsOnId);
}

void PaperDependencyWidget::removeDependency(int paperId, int dependsOnId) {
    if (!nodes_.contains(paperId)) return;
    nodes_[paperId].dependsOn.removeOne(dependsOnId);
    if (nodes_.contains(dependsOnId))
        nodes_[dependsOnId].requiredBy.removeOne(paperId);
    refreshTable();
    refreshTree();
}

void PaperDependencyWidget::buildFromPairs(const QList<QPair<int, int>>& deps) {
    for (const auto& [from, to] : deps) addDependency(from, to);
}

void PaperDependencyWidget::clear() {
    nodes_.clear();
    sourceCombo_->clear();
    targetCombo_->clear();
    refreshTable();
    refreshTree();
}

QList<DepNode> PaperDependencyWidget::topologicalSort() const {
    QMap<int, int> inDegree;
    for (const auto& node : nodes_) inDegree[node.paperId] = 0;
    for (const auto& node : nodes_) {
        for (int dep : node.dependsOn) {
            if (nodes_.contains(dep)) inDegree[dep] = inDegree.value(dep, 0);
        }
    }
    for (const auto& node : nodes_) {
        for (int dep : node.dependsOn) {
            if (nodes_.contains(dep)) inDegree[dep] = inDegree.value(dep, 0);
        }
    }
    // Compute in-degree (papers that depend on this paper)
    QMap<int, int> inD;
    for (const auto& node : nodes_) inD[node.paperId] = 0;
    for (const auto& node : nodes_) {
        for (int dep : node.dependsOn) {
            if (nodes_.contains(dep)) inD[node.paperId]++;
        }
    }

    QQueue<int> queue;
    for (auto it = inD.begin(); it != inD.end(); ++it) {
        if (it.value() == 0) queue.enqueue(it.key());
    }

    QList<DepNode> result;
    while (!queue.isEmpty()) {
        int id = queue.dequeue();
        result.append(nodes_[id]);
        for (int req : nodes_[id].requiredBy) {
            if (inD.contains(req)) {
                inD[req]--;
                if (inD[req] == 0) queue.enqueue(req);
            }
        }
    }
    return result;
}

QList<int> PaperDependencyWidget::getChain(int paperId) const {
    QList<int> chain;
    QSet<int> visited;
    QQueue<int> queue;
    queue.enqueue(paperId);
    while (!queue.isEmpty()) {
        int id = queue.dequeue();
        if (visited.contains(id)) continue;
        visited.insert(id);
        chain.append(id);
        if (nodes_.contains(id)) {
            for (int dep : nodes_[id].dependsOn) {
                if (!visited.contains(dep)) queue.enqueue(dep);
            }
        }
    }
    return chain;
}

int PaperDependencyWidget::dependencyDepth(int paperId) const {
    if (!nodes_.contains(paperId)) return 0;
    const auto& node = nodes_[paperId];
    if (node.dependsOn.isEmpty()) return 0;
    int maxD = 0;
    for (int dep : node.dependsOn) {
        maxD = qMax(maxD, dependencyDepth(dep) + 1);
    }
    return maxD;
}

void PaperDependencyWidget::onAddDep() {
    int from = sourceCombo_->currentData().toInt();
    int to = targetCombo_->currentData().toInt();
    if (from <= 0 || to <= 0 || from == to) return;
    addDependency(from, to);
    statsLabel_->setText(QString("Added dependency: %1 → %2").arg(from).arg(to));
}

void PaperDependencyWidget::onRemoveDep() {
    int row = depTable_->currentRow();
    if (row < 0) return;
    int from = sourceCombo_->currentData().toInt();
    int to = targetCombo_->currentData().toInt();
    removeDependency(from, to);
}

void PaperDependencyWidget::onCheckCycles() {
    QList<int> cycle;
    if (hasCycle()) {
        QSet<int> visited, recursion;
        dfs(nodes_.firstKey(), visited, recursion, cycle);
        statsLabel_->setText(QString("CYCLE DETECTED: %1").arg(
            QStringList() << "Cycle found" << ""));
        statsLabel_->setStyleSheet("font-size: 11px; color: #dc2626;");
        emit cycleDetected(cycle);
    } else {
        statsLabel_->setText("No cycles detected");
        statsLabel_->setStyleSheet("font-size: 11px; color: #059669;");
    }
}

void PaperDependencyWidget::onTopSort() {
    auto sorted = topologicalSort();
    depTree_->clear();
    for (const auto& node : sorted) {
        auto* item = new QTreeWidgetItem({
            node.title,
            QString::number(dependencyDepth(node.paperId)),
            QString::number(node.dependsOn.size())
        });
        item->setData(0, Qt::UserRole, node.paperId);
        if (node.dependsOn.isEmpty()) item->setForeground(0, QColor(5, 150, 105));
        depTree_->addTopLevelItem(item);
    }
    statsLabel_->setText(QString("Topological sort: %1 papers").arg(sorted.size()));
}

void PaperDependencyWidget::onNodeClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    int id = item->data(0, Qt::UserRole).toInt();
    QList<int> chain = getChain(id);
    emit chainSelected(id, chain);
}

void PaperDependencyWidget::onDepClicked(int row, int) {
    Q_UNUSED(row);
}

void PaperDependencyWidget::refreshTree() {
    depTree_->clear();
    for (const auto& node : nodes_) {
        auto* item = new QTreeWidgetItem({
            node.title,
            QString::number(dependencyDepth(node.paperId)),
            QString::number(node.dependsOn.size())
        });
        item->setData(0, Qt::UserRole, node.paperId);
        if (node.dependsOn.isEmpty()) item->setForeground(0, QColor(5, 150, 105));
        else if (dependencyDepth(node.paperId) > 3) item->setForeground(0, QColor(220, 38, 38));
        depTree_->addTopLevelItem(item);
    }
}

void PaperDependencyWidget::refreshTable() {
    depTable_->setRowCount(nodes_.size());
    int row = 0;
    for (const auto& node : nodes_) {
        depTable_->setItem(row, 0, new QTableWidgetItem(node.title));
        QStringList deps;
        for (int d : node.dependsOn) {
            if (nodes_.contains(d)) deps << nodes_[d].title;
        }
        depTable_->setItem(row, 1, new QTableWidgetItem(deps.join(", ")));

        QStringList reqs;
        for (int r : node.requiredBy) {
            if (nodes_.contains(r)) reqs << nodes_[r].title;
        }
        depTable_->setItem(row, 2, new QTableWidgetItem(reqs.join(", ")));
        row++;
    }
    statsLabel_->setText(QString("%1 papers, %2 dependencies").arg(nodes_.size()).arg(
        std::accumulate(nodes_.begin(), nodes_.end(), 0,
            [](int sum, const DepNode& n) { return sum + n.dependsOn.size(); })));
}

bool PaperDependencyWidget::hasCycle() const {
    QSet<int> visited, recursion;
    QList<int> cycle;
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        if (!visited.contains(it.key())) {
            if (dfs(it.key(), visited, recursion, cycle)) return true;
        }
    }
    return false;
}

bool PaperDependencyWidget::dfs(int node, QSet<int>& visited, QSet<int>& recursionStack, QList<int>& cycle) const {
    visited.insert(node);
    recursionStack.insert(node);

    if (nodes_.contains(node)) {
        for (int dep : nodes_[node].dependsOn) {
            if (!visited.contains(dep)) {
                if (dfs(dep, visited, recursionStack, cycle)) return true;
            } else if (recursionStack.contains(dep)) {
                cycle.append(dep);
                return true;
            }
        }
    }

    recursionStack.remove(node);
    return false;
}
