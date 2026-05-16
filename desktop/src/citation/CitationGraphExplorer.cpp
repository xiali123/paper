#include "citation/CitationGraphExplorer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QSpinBox>
#include <cmath>

CitationGraphExplorer::CitationGraphExplorer(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void CitationGraphExplorer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Layout:"));
    layoutCombo_ = new QComboBox();
    layoutCombo_->addItems({"Force", "Radial", "Grid", "Random"});
    connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CitationGraphExplorer::onLayoutChanged);
    toolbar->addWidget(layoutCombo_);

    toolbar->addWidget(new QLabel("Depth:"));
    depthCombo_ = new QComboBox();
    depthCombo_->addItems({"All", "1", "2", "3"});
    connect(depthCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CitationGraphExplorer::onDepthChanged);
    toolbar->addWidget(depthCombo_);

    fitBtn_ = new QPushButton("Fit");
    connect(fitBtn_, &QPushButton::clicked, this, &CitationGraphExplorer::onFitView);
    toolbar->addWidget(fitBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);
    auto* canvas = new QWidget();
    canvas->setMinimumSize(450, 350);
    canvas->setMouseTracking(true);
    splitter->addWidget(canvas);

    nodeList_ = new QListWidget();
    nodeList_->setMaximumWidth(180);
    nodeList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 2px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(nodeList_, &QListWidget::itemClicked, this, &CitationGraphExplorer::onNodeListClicked);
    splitter->addWidget(nodeList_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("Load papers to explore citation graph");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void CitationGraphExplorer::loadFromPapers(const QList<QPair<int, QString>>& papers) {
    nodes_.clear();
    links_.clear();
    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                           QColor(139,92,246), QColor(236,72,153), QColor(14,165,233)};
    for (int i = 0; i < papers.size(); ++i) {
        GraphNode n;
        n.id = papers[i].first;
        n.label = papers[i].second;
        n.pos = QPointF(QRandomGenerator::global()->bounded(200.0) - 100,
                        QRandomGenerator::global()->bounded(200.0) - 100);
        n.color = typeColors[i % 6];
        n.size = 12 + QRandomGenerator::global()->bounded(10);
        n.depth = QRandomGenerator::global()->bounded(4);
        nodes_.append(n);
    }
    for (int i = 0; i < qMin(nodes_.size(), 30); ++i) {
        int from = QRandomGenerator::global()->bounded(nodes_.size());
        int to = QRandomGenerator::global()->bounded(nodes_.size());
        if (from != to) addLink(nodes_[from].id, nodes_[to].id);
    }

    QString method = layoutCombo_->currentText();
    if (method == "Force") forceLayout();
    else if (method == "Radial") radialLayout();

    statsLabel_->setText(QString("%1 nodes, %2 links").arg(nodes_.size()).arg(links_.size()));
    emit explorationComplete(nodes_.size(), links_.size());
    update();
}

void CitationGraphExplorer::addNode(const GraphNode& node) {
    nodes_.append(node);
    update();
}

void CitationGraphExplorer::addLink(int source, int target, const QString& relation, qreal weight) {
    GraphLink l;
    l.source = source;
    l.target = target;
    l.relation = relation;
    l.weight = weight;
    links_.append(l);
}

void CitationGraphExplorer::clear() {
    nodes_.clear();
    links_.clear();
    nodeList_->clear();
    update();
}

QList<GraphNode> CitationGraphExplorer::nodes() const { return nodes_; }
QList<GraphLink> CitationGraphExplorer::links() const { return links_; }

void CitationGraphExplorer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(15, 23, 42));

    p.save();
    p.translate(offset_);
    p.scale(zoom_, zoom_);

    // Links
    for (const auto& l : links_) {
        QPointF from, to;
        bool f1 = false, f2 = false;
        for (const auto& n : nodes_) {
            if (n.id == l.source) { from = n.pos; f1 = true; }
            if (n.id == l.target) { to = n.pos; f2 = true; }
        }
        if (!f1 || !f2) continue;

        QColor linkColor = (selectedNode_ == l.source || selectedNode_ == l.target)
            ? QColor(59, 130, 246, 200) : QColor(100, 116, 139, 80);
        p.setPen(QPen(linkColor, l.weight));
        p.drawLine(from, to);
    }

    // Nodes
    for (const auto& n : nodes_) {
        bool isSelected = (n.id == selectedNode_);
        p.setPen(Qt::NoPen);
        if (isSelected) {
            p.setBrush(n.color.lighter(130));
            p.drawEllipse(n.pos, n.size + 4, n.size + 4);
        }
        p.setBrush(n.color);
        p.drawEllipse(n.pos, n.size, n.size);

        // Label for selected
        if (isSelected) {
            p.setPen(QPen(Qt::white, 1));
            p.setFont(QFont("Arial", 8));
            p.drawText(QRectF(n.pos.x() - 60, n.pos.y() - n.size - 16, 120, 14),
                       Qt::AlignCenter, n.label.left(20));
        }
    }

    p.restore();
}

void CitationGraphExplorer::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = (event->pos() - offset_) / zoom_;
    int hit = hitTestNode(scenePos);
    if (hit >= 0) {
        selectedNode_ = hit;
        for (const auto& n : nodes_) {
            if (n.id == hit) {
                emit nodeSelected(hit, n.label);
                statsLabel_->setText(QString("Node: %1 (depth %2)").arg(n.label.left(40)).arg(n.depth));
                break;
            }
        }
        update();
    } else {
        panning_ = true;
    }
    lastMousePos_ = event->pos();
}

void CitationGraphExplorer::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        offset_ += event->pos() - lastMousePos_;
        update();
    }
    lastMousePos_ = event->pos();
}

void CitationGraphExplorer::wheelEvent(QWheelEvent* event) {
    qreal factor = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
    zoom_ = qBound(0.1, zoom_ * factor, 5.0);
    update();
}

int CitationGraphExplorer::hitTestNode(const QPointF& pos) {
    for (const auto& n : nodes_) {
        qreal dx = pos.x() - n.pos.x();
        qreal dy = pos.y() - n.pos.y();
        if (dx * dx + dy * dy <= n.size * n.size) return n.id;
    }
    return -1;
}

void CitationGraphExplorer::forceLayout() {
    for (auto& n : nodes_) {
        n.pos = QPointF(QRandomGenerator::global()->bounded(200.0) - 100,
                        QRandomGenerator::global()->bounded(200.0) - 100);
    }
    for (int iter = 0; iter < 60; ++iter) {
        for (int i = 0; i < nodes_.size(); ++i) {
            QPointF force(0, 0);
            for (int j = 0; j < nodes_.size(); ++j) {
                if (i == j) continue;
                QPointF diff = nodes_[i].pos - nodes_[j].pos;
                qreal dist = qMax(1.0, std::sqrt(diff.x() * diff.x() + diff.y() * diff.y()));
                force += diff * (300.0 / (dist * dist));
            }
            for (const auto& l : links_) {
                int other = -1;
                if (l.source == nodes_[i].id) other = l.target;
                else if (l.target == nodes_[i].id) other = l.source;
                if (other < 0) continue;
                for (auto& n : nodes_) {
                    if (n.id == other) {
                        QPointF diff = n.pos - nodes_[i].pos;
                        qreal dist = qMax(1.0, std::sqrt(diff.x() * diff.x() + diff.y() * diff.y()));
                        force += diff * (dist - 70) * 0.008;
                        break;
                    }
                }
            }
            nodes_[i].pos += force * 0.08;
        }
    }

    nodeList_->clear();
    for (const auto& n : nodes_) {
        auto* item = new QListWidgetItem(QString("[%1] %2").arg(n.depth).arg(n.label.left(25)));
        item->setData(Qt::UserRole, n.id);
        nodeList_->addItem(item);
    }
}

void CitationGraphExplorer::radialLayout() {
    QMap<int, QList<GraphNode*>> rings;
    for (auto& n : nodes_) rings[n.depth].append(&n);
    for (auto it = rings.begin(); it != rings.end(); ++it) {
        qreal radius = 40 + it.key() * 80;
        auto& items = it.value();
        qreal step = 2.0 * M_PI / qMax(1, items.size());
        qreal angle = 0;
        for (auto* n : items) {
            n->pos = QPointF(radius * std::cos(angle), radius * std::sin(angle));
            angle += step;
        }
    }
    update();
}

void CitationGraphExplorer::onLayoutChanged(int) {
    QString method = layoutCombo_->currentText();
    if (method == "Force") forceLayout();
    else if (method == "Radial") radialLayout();
    else if (method == "Grid") {
        int cols = qMax(1, static_cast<int>(std::sqrt(nodes_.size())));
        for (int i = 0; i < nodes_.size(); ++i) {
            nodes_[i].pos = QPointF((i % cols) * 70, (i / cols) * 70);
        }
        update();
    } else {
        for (auto& n : nodes_) {
            n.pos = QPointF(QRandomGenerator::global()->bounded(300.0) - 150,
                            QRandomGenerator::global()->bounded(300.0) - 150);
        }
        update();
    }
}

void CitationGraphExplorer::onDepthChanged(int) { update(); }
void CitationGraphExplorer::onNodeListClicked() {
    auto* item = nodeList_->currentItem();
    if (!item) return;
    selectedNode_ = item->data(Qt::UserRole).toInt();
    update();
}

void CitationGraphExplorer::onFitView() {
    if (nodes_.isEmpty()) return;
    qreal minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (const auto& n : nodes_) {
        minX = qMin(minX, n.pos.x() - n.size);
        minY = qMin(minY, n.pos.y() - n.size);
        maxX = qMax(maxX, n.pos.x() + n.size);
        maxY = qMax(maxY, n.pos.y() + n.size);
    }
    qreal w = maxX - minX + 60, h = maxY - minY + 60;
    zoom_ = qMin(width() * 0.7 / w, height() * 0.7 / h);
    zoom_ = qBound(0.1, zoom_, 5.0);
    offset_ = QPointF(width() / 2 - (minX + w / 2) * zoom_, height() / 2 - (minY + h / 2) * zoom_);
    update();
}
