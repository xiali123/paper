#include "visualization/PaperNetworkGraph.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <cmath>

PaperNetworkGraph::PaperNetworkGraph(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperNetworkGraph::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    layoutCombo_ = new QComboBox();
    layoutCombo_->addItems({"Force-Directed", "Circular", "Grouped"});
    connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperNetworkGraph::onLayoutChanged);
    toolbar->addWidget(new QLabel("Layout:"));
    toolbar->addWidget(layoutCombo_, 1);

    fitBtn_ = new QPushButton("Fit View");
    connect(fitBtn_, &QPushButton::clicked, this, &PaperNetworkGraph::onFitView);
    toolbar->addWidget(fitBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Canvas
    auto* canvas = new QWidget();
    canvas->setMinimumSize(400, 300);
    canvas->setMouseTracking(true);
    splitter->addWidget(canvas);

    // Legend
    legendList_ = new QListWidget();
    legendList_->setMaximumWidth(180);
    legendList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
    );
    splitter->addWidget(legendList_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("Add authors to build network");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperNetworkGraph::addAuthor(const QString& name) {
    if (nodes_.contains(name)) return;
    NetworkNode node;
    node.id = nextNodeId_++;
    node.name = name;
    node.type = "author";
    node.pos = QPointF(150 + (qrand() % 300) - 150, 150 + (qrand() % 300) - 150);
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246), QColor(236,72,153)};
    node.color = colors[nextNodeId_ % 5];
    nodes_[name] = node;
}

void PaperNetworkGraph::addCoauthorship(const QString& author1, const QString& author2, qreal weight) {
    addAuthor(author1);
    addAuthor(author2);
    NetworkEdge edge;
    edge.from = nodes_[author1].id;
    edge.to = nodes_[author2].id;
    edge.weight = weight;
    edge.label = QString::number(weight, 'f', 0);
    edges_.append(edge);
    nodes_[author1].degree++;
    nodes_[author2].degree++;

    legendList_->clear();
    QMap<QString, int> sorted;
    for (const auto& n : nodes_) sorted[n.name] = n.degree;
    for (auto it = sorted.begin(); it != sorted.end(); ++it) {
        auto* item = new QListWidgetItem(QString("%1 (%2 co-authors)").arg(it.key()).arg(it.value()));
        if (nodes_.contains(it.key())) item->setForeground(nodes_[it.key()].color);
        legendList_->addItem(item);
    }

    statsLabel_->setText(QString("%1 authors, %2 connections").arg(nodes_.size()).arg(edges_.size()));
}

void PaperNetworkGraph::buildFromPapers(const QList<QPair<int, QStringList>>& papers) {
    for (const auto& [paperId, authors] : papers) {
        Q_UNUSED(paperId);
        for (const auto& author : authors) addAuthor(author);
        for (int i = 0; i < authors.size(); ++i) {
            for (int j = i + 1; j < authors.size(); ++j) {
                addCoauthorship(authors[i], authors[j], 1.0);
            }
        }
    }
    autoLayout();
}

void PaperNetworkGraph::clearGraph() {
    nodes_.clear();
    edges_.clear();
    legendList_->clear();
    nextNodeId_ = 1;
    update();
}

void PaperNetworkGraph::autoLayout() {
    onLayoutChanged(layoutCombo_->currentIndex());
}

void PaperNetworkGraph::setNodeSizeByDegree(bool enabled) {
    sizeByDegree_ = enabled;
    for (auto& node : nodes_) {
        node.radius = sizeByDegree_ ? qMax(10.0, 8.0 + node.degree * 2.0) : 16.0;
    }
    update();
}

void PaperNetworkGraph::onLayoutChanged(int index) {
    switch (index) {
        case 0: layoutForce(); break;
        case 1: layoutCircular(); break;
        case 2: layoutGrouped(); break;
    }
    update();
    emit layoutApplied();
}

void PaperNetworkGraph::onFitView() {
    if (nodes_.isEmpty()) return;
    qreal minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (const auto& n : nodes_) {
        minX = qMin(minX, n.pos.x()); minY = qMin(minY, n.pos.y());
        maxX = qMax(maxX, n.pos.x()); maxY = qMax(maxY, n.pos.y());
    }
    qreal w = maxX - minX + 80, h = maxY - minY + 80;
    zoom_ = qMin(width() * 0.7 / w, height() * 0.7 / h);
    zoom_ = qBound(0.2, zoom_, 3.0);
    offset_ = QPointF(width() / 2 - (minX + w / 2) * zoom_, height() / 2 - (minY + h / 2) * zoom_);
    update();
}

void PaperNetworkGraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    p.save();
    p.translate(offset_);
    p.scale(zoom_, zoom_);

    // Edges
    for (const auto& edge : edges_) {
        const NetworkNode* a = nullptr, * b = nullptr;
        for (const auto& n : nodes_) {
            if (n.id == edge.from) a = &n;
            if (n.id == edge.to) b = &n;
        }
        if (!a || !b) continue;

        QPen pen(QColor(200, 200, 210), qMax(1.0, edge.weight * 0.8));
        p.setPen(pen);
        p.drawLine(a->pos, b->pos);
    }

    // Nodes
    for (const auto& node : nodes_) {
        qreal r = sizeByDegree_ ? qMax(10.0, 8.0 + node.degree * 2.0) : 16.0;

        // Glow
        p.setPen(Qt::NoPen);
        p.setBrush(node.color.lighter(160));
        p.drawEllipse(node.pos, r + 3, r + 3);

        // Body
        p.setBrush(node.color);
        p.drawEllipse(node.pos, r, r);

        // Label
        p.setPen(QColor(30, 41, 59));
        QFont font = p.font();
        font.setPixelSize(qMax(8, static_cast<int>(r * 0.7)));
        font.setBold(true);
        p.setFont(font);
        p.drawText(QRectF(node.pos.x() - r * 2, node.pos.y() + r + 2, r * 4, 14),
                   Qt::AlignCenter, node.name.left(20));
    }

    p.restore();
}

void PaperNetworkGraph::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = (event->pos() - offset_) / zoom_;
    int hit = hitTest(scenePos);
    if (hit >= 0) {
        dragNode_ = hit;
        dragging_ = true;
        for (const auto& n : nodes_) {
            if (n.id == hit) {
                emit nodeClicked(n.name, n.type);
                break;
            }
        }
    }
    lastMousePos_ = event->pos();
}

void PaperNetworkGraph::mouseMoveEvent(QMouseEvent* event) {
    if (dragging_ && dragNode_ >= 0) {
        QPointF scenePos = (event->pos() - offset_) / zoom_;
        for (auto& n : nodes_) {
            if (n.id == dragNode_) { n.pos = scenePos; break; }
        }
        update();
    } else {
        QPointF delta = event->pos() - lastMousePos_;
        offset_ += delta;
        update();
    }
    lastMousePos_ = event->pos();
}

void PaperNetworkGraph::wheelEvent(QWheelEvent* event) {
    qreal factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
    zoom_ = qBound(0.2, zoom_ * factor, 3.0);
    update();
}

int PaperNetworkGraph::hitTest(const QPointF& pos) {
    for (const auto& node : nodes_) {
        qreal r = sizeByDegree_ ? qMax(10.0, 8.0 + node.degree * 2.0) : 16.0;
        qreal dx = pos.x() - node.pos.x();
        qreal dy = pos.y() - node.pos.y();
        if (dx * dx + dy * dy <= r * r) return node.id;
    }
    return -1;
}

void PaperNetworkGraph::layoutForce() {
    if (nodes_.size() < 2) { layoutCircular(); return; }
    QList<int> ids;
    for (const auto& n : nodes_) ids.append(n.id);

    for (int iter = 0; iter < 80; ++iter) {
        QMap<int, QPointF> forces;
        for (int id : ids) forces[id] = QPointF(0, 0);

        for (int i = 0; i < ids.size(); ++i) {
            for (int j = i + 1; j < ids.size(); ++j) {
                const auto& na = nodes_.values()[ids[i] < nodes_.size() ? ids[i] : 0];
                const auto& nb = nodes_.values()[ids[j] < nodes_.size() ? ids[j] : 0];
                QPointF diff = na.pos - nb.pos;
                qreal dist = qMax(1.0, sqrt(diff.x()*diff.x() + diff.y()*diff.y()));
                qreal f = 6000.0 / (dist * dist);
                QPointF unit(diff.x()/dist, diff.y()/dist);
                forces[ids[i]] += unit * f;
                forces[ids[j]] -= unit * f;
            }
        }

        for (const auto& edge : edges_) {
            const NetworkNode* a = nullptr, * b = nullptr;
            for (const auto& n : nodes_) {
                if (n.id == edge.from) a = &n;
                if (n.id == edge.to) b = &n;
            }
            if (!a || !b) continue;
            QPointF diff = b->pos - a->pos;
            qreal dist = qMax(1.0, sqrt(diff.x()*diff.x() + diff.y()*diff.y()));
            qreal f = (dist - 100.0) * 0.04;
            QPointF unit(diff.x()/dist, diff.y()/dist);
            forces[edge.from] += unit * f;
            forces[edge.to] -= unit * f;
        }

        QList<NetworkNode> nodeList = nodes_.values();
        for (auto& node : nodeList) {
            if (!forces.contains(node.id)) continue;
            QPointF f = forces[node.id];
            qreal mag = sqrt(f.x()*f.x() + f.y()*f.y());
            if (mag > 5.0) f *= 5.0 / mag;
            node.pos += f;
            nodes_[node.name] = node;
        }
    }
    update();
}

void PaperNetworkGraph::layoutCircular() {
    int n = nodes_.size();
    if (n == 0) return;
    qreal r = qMax(80.0, n * 12.0);
    int i = 0;
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        qreal angle = 2.0 * M_PI * i / n;
        it->pos = QPointF(r * cos(angle), r * sin(angle));
        i++;
    }
    update();
}

void PaperNetworkGraph::layoutGrouped() {
    layoutForce();
}
