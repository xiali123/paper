#include "citation/PaperCitationNetwork.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperCitationNetwork::PaperCitationNetwork(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperCitationNetwork::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate Network");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperCitationNetwork::onGenerate);
    toolbar->addWidget(generateBtn_);

    layoutBtn_ = new QPushButton("Relayout");
    connect(layoutBtn_, &QPushButton::clicked, this, &PaperCitationNetwork::onLayout);
    toolbar->addWidget(layoutBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationNetwork::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Generate citation network");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCitationNetwork::addNode(const CitationNode& node) {
    nodes_.append(node);
    emit networkUpdated(nodes_.size(), edges_.size());
    update();
}

void PaperCitationNetwork::addEdge(int from, int to, qreal weight) {
    CitationEdge e;
    e.from = from;
    e.to = to;
    e.weight = weight;
    edges_.append(e);
    update();
}

QList<CitationNode> PaperCitationNetwork::nodes() const { return nodes_; }
QList<CitationEdge> PaperCitationNetwork::edges() const { return edges_; }

QMap<QString, int> PaperCitationNetwork::clusterCounts() const {
    QMap<QString, int> counts;
    for (const auto& n : nodes_) counts[n.cluster]++;
    return counts;
}

void PaperCitationNetwork::onGenerate() {
    nodes_.clear();
    edges_.clear();

    QStringList clusters = {"NLP", "Vision", "RL", "Theory", "Systems"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};
    QStringList titles = {
        "Attention Is All You Need", "BERT", "GPT-3", "ResNet", "ViT",
        "DQN", "PPO", "GAN", "Diffusion", "Transformer-XL",
        "CLIP", "DALL-E", "LLaMA", "ResNet-50", "AlphaGo",
        "Inception", "YOLO", "EfficientNet", "BART", "T5"};

    int count = 15 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        CitationNode n;
        n.id = i;
        n.title = titles[i % titles.size()];
        n.year = 2018 + QRandomGenerator::global()->bounded(8);
        n.citations = 50 + QRandomGenerator::global()->bounded(950);
        n.cluster = clusters[i % clusters.size()];
        n.color = colors[i % 5];
        n.x = 100 + QRandomGenerator::global()->bounded(400);
        n.y = 100 + QRandomGenerator::global()->bounded(300);
        nodes_.append(n);
    }

    for (int i = 0; i < count; ++i) {
        int numEdges = 1 + QRandomGenerator::global()->bounded(3);
        for (int j = 0; j < numEdges; ++j) {
            int target = QRandomGenerator::global()->bounded(count);
            if (target != i) {
                bool exists = false;
                for (const auto& e : edges_) {
                    if ((e.from == i && e.to == target) || (e.from == target && e.to == i)) { exists = true; break; }
                }
                if (!exists) addEdge(i, target, 0.3 + QRandomGenerator::global()->bounded(70) / 100.0);
            }
        }
    }

    forceDirectedLayout(80);
    updateInfo();
    emit networkUpdated(nodes_.size(), edges_.size());
    update();
}

void PaperCitationNetwork::onLayout() {
    forceDirectedLayout(100);
    update();
}

void PaperCitationNetwork::onClear() {
    nodes_.clear();
    edges_.clear();
    selectedNode_ = -1;
    infoLabel_->setText("Generate citation network");
    update();
}

void PaperCitationNetwork::forceDirectedLayout(int iterations) {
    if (nodes_.size() < 2) return;
    qreal w = width(), h = height();
    for (int iter = 0; iter < iterations; ++iter) {
        qreal temp = qreal(iterations - iter) / iterations * 50;
        for (int i = 0; i < nodes_.size(); ++i) {
            qreal fx = 0, fy = 0;
            for (int j = 0; j < nodes_.size(); ++j) {
                if (i == j) continue;
                qreal dx = nodes_[i].x - nodes_[j].x;
                qreal dy = nodes_[i].y - nodes_[j].y;
                qreal dist = qMax(1.0, std::sqrt(dx * dx + dy * dy));
                qreal force = 2000.0 / (dist * dist);
                fx += (dx / dist) * force;
                fy += (dy / dist) * force;
            }
            for (const auto& e : edges_) {
                int other = -1;
                if (e.from == i) other = e.to;
                else if (e.to == i) other = e.from;
                if (other < 0) continue;
                qreal dx = nodes_[i].x - nodes_[other].x;
                qreal dy = nodes_[i].y - nodes_[other].y;
                qreal dist = qMax(1.0, std::sqrt(dx * dx + dy * dy));
                qreal force = (dist - 80) * 0.05;
                fx -= (dx / dist) * force;
                fy -= (dy / dist) * force;
            }
            nodes_[i].vx = (nodes_[i].vx + fx) * 0.9;
            nodes_[i].vy = (nodes_[i].vy + fy) * 0.9;
            nodes_[i].x += nodes_[i].vx * temp * 0.01;
            nodes_[i].y += nodes_[i].vy * temp * 0.01;
            nodes_[i].x = qBound(40.0, nodes_[i].x, w - 40.0);
            nodes_[i].y = qBound(60.0, nodes_[i].y, h - 60.0);
        }
    }
}

void PaperCitationNetwork::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (nodes_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate citation network");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Network");

    int w = width(), h = height();
    drawNetwork(p, QRect(20, 50, w / 3 * 2, h - 80));
    drawLegend(p, QRect(w / 3 * 2 + 10, 50, w / 3 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 3 * 2 + 10, h / 2 + 20, w / 3 - 30, h / 2 - 50));
}

void PaperCitationNetwork::drawNetwork(QPainter& p, const QRect& rect) {
    for (const auto& e : edges_) {
        if (e.from < 0 || e.from >= nodes_.size() || e.to < 0 || e.to >= nodes_.size()) continue;
        p.setPen(QPen(QColor(203, 213, 225), qMax(1.0, e.weight * 2)));
        p.drawLine(static_cast<int>(nodes_[e.from].x), static_cast<int>(nodes_[e.from].y),
                   static_cast<int>(nodes_[e.to].x), static_cast<int>(nodes_[e.to].y));
    }

    for (const auto& n : nodes_) {
        int radius = qBound(6, 4 + n.citations / 50, 20);
        bool sel = (n.id == selectedNode_);

        p.setPen(Qt::NoPen);
        p.setBrush(sel ? n.color : n.color.lighter(130));
        p.drawEllipse(QPointF(n.x, n.y), radius, radius);

        if (sel) {
            p.setPen(QPen(n.color, 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(n.x, n.y), radius + 4, radius + 4);
        }

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(static_cast<int>(n.x) - 30, static_cast<int>(n.y) + radius + 10, 60, 12,
                   Qt::AlignCenter, n.title.left(10));
    }
}

void PaperCitationNetwork::drawLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Clusters");

    auto counts = clusterCounts();
    QStringList clusters = counts.keys();
    int itemH = qMin(24, (rect.height() - 25) / qMax(1, clusters.size()));

    for (int i = 0; i < clusters.size(); ++i) {
        int y = rect.y() + 22 + i * itemH;
        const auto& n = nodes_.first();
        QColor c;
        for (const auto& nd : nodes_) { if (nd.cluster == clusters[i]) { c = nd.color; break; } }

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(rect.x() + 5, y + itemH / 2 - 5, 10, 10);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 20, y, rect.width() - 50, itemH, Qt::AlignVCenter, clusters[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() - 30, y, 30, itemH, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(counts[clusters[i]]));
    }
}

void PaperCitationNetwork::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Nodes", QString::number(nodes_.size()), QColor(59,130,246)},
        {"Edges", QString::number(edges_.size()), QColor(16,185,129)},
        {"Clusters", QString::number(clusterCounts().size()), QColor(245,158,11)},
        {"Density", QString::number(nodes_.size() > 1 ? (2.0 * edges_.size()) / (nodes_.size() * (nodes_.size() - 1)) : 0, 'f', 2), QColor(139,92,246)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperCitationNetwork::updateInfo() {
    if (nodes_.isEmpty()) { infoLabel_->setText("Generate citation network"); return; }
    qreal density = nodes_.size() > 1 ? (2.0 * edges_.size()) / (nodes_.size() * (nodes_.size() - 1)) : 0;
    infoLabel_->setText(QString("%1 nodes | %2 edges | %3 clusters | density: %4")
        .arg(nodes_.size()).arg(edges_.size()).arg(clusterCounts().size())
        .arg(density, 0, 'f', 2));
}
