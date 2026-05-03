#include "PaperClusteringWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QPainter>
#include <algorithm>
#include <random>

PaperClusteringWidget::PaperClusteringWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperClusteringWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* header = new QLabel("Paper Clustering");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(header);

    // Controls
    auto* ctrlRow = new QHBoxLayout();
    ctrlRow->addWidget(new QLabel("Clusters (k):"));

    auto* kSpin = new QSpinBox();
    kSpin->setRange(2, 20);
    kSpin->setValue(k_);
    connect(kSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PaperClusteringWidget::onClusterCountChanged);
    ctrlRow->addWidget(kSpin);

    ctrlRow->addWidget(new QLabel("Method:"));
    methodCombo_ = new QComboBox();
    methodCombo_->addItems({"Keyword Similarity", "TF-IDF", "Co-citation"});
    ctrlRow->addWidget(methodCombo_, 1);

    runBtn_ = new QPushButton("Run Clustering");
    runBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(runBtn_, &QPushButton::clicked, this, &PaperClusteringWidget::onRunClustering);
    ctrlRow->addWidget(runBtn_);

    layout->addLayout(ctrlRow);

    statsLabel_ = new QLabel("Load papers and run clustering");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    clusterList_ = new QListWidget();
    clusterList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(clusterList_, &QListWidget::itemClicked, this, &PaperClusteringWidget::onItemClicked);
    layout->addWidget(clusterList_, 1);
}

void PaperClusteringWidget::setPapers(const QList<QPair<int, QString>>& papers) {
    nodes_.clear();
    for (const auto& [id, keywords] : papers) {
        ClusterNode node;
        node.paperId = id;
        node.keywords = keywords;
        node.cluster = -1;
        nodes_.append(node);
    }
    statsLabel_->setText(QString("%1 papers loaded").arg(nodes_.size()));
}

void PaperClusteringWidget::setClusterCount(int k) {
    k_ = qBound(2, k, 20);
}

QList<ClusterNode> PaperClusteringWidget::clusters() const {
    return nodes_;
}

QMap<int, QList<int>> PaperClusteringWidget::clusterGroups() const {
    QMap<int, QList<int>> groups;
    for (const auto& node : nodes_) {
        groups[node.cluster].append(node.paperId);
    }
    return groups;
}

void PaperClusteringWidget::onRunClustering() {
    if (nodes_.isEmpty()) {
        statsLabel_->setText("No papers loaded");
        return;
    }
    runKMeans();
    refreshClusterList();
    auto groups = clusterGroups();
    emit clusteringDone(k_, groups);
    statsLabel_->setText(QString("Clustering done: %1 clusters from %2 papers")
        .arg(k_).arg(nodes_.size()));
}

void PaperClusteringWidget::onClusterCountChanged(int value) {
    k_ = value;
}

void PaperClusteringWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    if (id < 0) {
        emit clusterClicked(-id); // negative = cluster id
    } else {
        emit paperClicked(id);
    }
}

void PaperClusteringWidget::runKMeans() {
    int n = nodes_.size();
    if (n == 0) return;

    // Initialize cluster colors
    clusterColors_.clear();
    for (int i = 0; i < k_; ++i) {
        clusterColors_[i] = QColor::fromHsv(i * 360 / k_, 180, 200);
    }

    // Random initialization
    std::mt19937 rng(42);
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    // Pick k centroids
    QList<int> centroidIdx;
    for (int i = 0; i < qMin(k_, n); ++i) {
        centroidIdx.append(indices[i]);
    }

    // Iterate
    for (int iter = 0; iter < 50; ++iter) {
        bool changed = false;

        // Assign each node to nearest centroid
        for (auto& node : nodes_) {
            double bestSim = -1;
            int bestCluster = 0;
            for (int c = 0; c < centroidIdx.size(); ++c) {
                double sim = similarity(node.keywords, nodes_[centroidIdx[c]].keywords);
                if (sim > bestSim) {
                    bestSim = sim;
                    bestCluster = c;
                }
            }
            if (node.cluster != bestCluster) {
                node.cluster = bestCluster;
                changed = true;
            }
        }

        if (!changed) break;
    }

    // Handle nodes that didn't get assigned
    for (auto& node : nodes_) {
        if (node.cluster < 0) node.cluster = 0;
    }
}

double PaperClusteringWidget::similarity(const QString& a, const QString& b) const {
    if (a.isEmpty() || b.isEmpty()) return 0.0;

    QStringList wordsA = a.toLower().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);
    QStringList wordsB = b.toLower().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts);

    QSet<QString> setA(wordsA.begin(), wordsA.end());
    QSet<QString> setB(wordsB.begin(), wordsB.end());

    int intersection = 0;
    for (const auto& w : setA) {
        if (setB.contains(w)) intersection++;
    }

    int unionSize = setA.size() + setB.size() - intersection;
    return (unionSize == 0) ? 0.0 : static_cast<double>(intersection) / unionSize;
}

void PaperClusteringWidget::refreshClusterList() {
    clusterList_->clear();
    auto groups = clusterGroups();

    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
        QColor color = clusterColors_.value(it.key(), Qt::gray);

        auto* header = new QListWidgetItem(
            QString("Cluster %1 (%2 papers)").arg(it.key() + 1).arg(it.value().size()));
        header->setData(Qt::UserRole, -it.key());
        header->setForeground(color);
        QFont font;
        font.setBold(true);
        header->setFont(font);
        clusterList_->addItem(header);

        for (int paperId : it.value()) {
            for (const auto& node : nodes_) {
                if (node.paperId == paperId) {
                    auto* item = new QListWidgetItem("   Paper #" + QString::number(paperId));
                    item->setData(Qt::UserRole, paperId);
                    item->setForeground(color);
                    clusterList_->addItem(item);
                    break;
                }
            }
        }
    }
}
