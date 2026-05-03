#include "PaperMindMapWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <cmath>
#include <random>

PaperMindMapWidget::PaperMindMapWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperMindMapWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    layoutCombo_ = new QComboBox();
    layoutCombo_->addItems({"Radial", "Force-Directed", "Tree"});
    connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperMindMapWidget::onLayoutChanged);
    toolbar->addWidget(new QLabel("Layout:"));
    toolbar->addWidget(layoutCombo_, 1);

    zoomSlider_ = new QSlider(Qt::Horizontal);
    zoomSlider_->setRange(25, 300);
    zoomSlider_->setValue(100);
    zoomSlider_->setMaximumWidth(150);
    connect(zoomSlider_, &QSlider::valueChanged, this, &PaperMindMapWidget::onZoomChanged);
    toolbar->addWidget(new QLabel("Zoom:"));
    toolbar->addWidget(zoomSlider_);

    fitBtn_ = new QPushButton("Fit");
    connect(fitBtn_, &QPushButton::clicked, this, &PaperMindMapWidget::onFitView);
    toolbar->addWidget(fitBtn_);

    layout->addLayout(toolbar);

    splitter_ = new QSplitter(Qt::Horizontal);

    outlineTree_ = new QTreeWidget();
    outlineTree_->setHeaderLabels({"Node", "ID"});
    outlineTree_->setMaximumWidth(200);
    outlineTree_->setColumnWidth(0, 130);
    splitter_->addWidget(outlineTree_);

    canvas_ = new QWidget();
    canvas_->setMinimumSize(400, 300);
    canvas_->setMouseTracking(true);
    splitter_->addWidget(canvas_);

    splitter_->setStretchFactor(0, 0);
    splitter_->setStretchFactor(1, 1);
    layout->addWidget(splitter_, 1);

    infoLabel_ = new QLabel("Add papers to build mind map");
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(infoLabel_);
}

void PaperMindMapWidget::setPapers(const QList<QPair<int, QString>>& papers) {
    for (const auto& [pid, title] : papers) {
        if (nodes_.contains(pid)) continue;
        MindMapNode node;
        node.id = pid;
        node.label = title.length() > 30 ? title.left(27) + "..." : title;
        node.paperId = pid;
        node.pos = QPointF(
            200 + (qrand() % 300) - 150,
            200 + (qrand() % 300) - 150
        );
        QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};
        node.color = colors[pid % 5];
        nodes_[pid] = node;
    }
    autoLayout();
    infoLabel_->setText(QString("%1 nodes, %2 edges").arg(nodes_.size()).arg(edges_.size()));
}

void PaperMindMapWidget::addRelation(int paperA, int paperB, const QString& relation) {
    MindMapEdge edge;
    edge.from = paperA;
    edge.to = paperB;
    edge.label = relation;
    edge.color = (relation == "cites") ? QColor(59,130,246) :
                 (relation == "related") ? QColor(16,185,129) : QColor(180,180,180);
    edges_.append(edge);
    if (nodes_.contains(paperA))
        nodes_[paperA].children.append(paperB);
}

void PaperMindMapWidget::clearMap() {
    nodes_.clear();
    edges_.clear();
    outlineTree_->clear();
    update();
    infoLabel_->setText("Map cleared");
}

void PaperMindMapWidget::autoLayout() {
    int idx = layoutCombo_->currentIndex();
    switch (idx) {
        case 0: layoutRadial(); break;
        case 1: layoutForceDirected(); break;
        case 2: layoutTree(); break;
    }
    emit layoutChanged();
}

void PaperMindMapWidget::expandAll() {
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
        it->expanded = true;
    update();
}

void PaperMindMapWidget::collapseAll() {
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
        it->expanded = false;
    update();
}

void PaperMindMapWidget::onLayoutChanged(int index) {
    Q_UNUSED(index);
    autoLayout();
    canvas_->update();
}

void PaperMindMapWidget::onZoomChanged(int value) {
    zoom_ = value / 100.0;
    canvas_->update();
}

void PaperMindMapWidget::onFitView() {
    if (nodes_.isEmpty()) return;
    qreal minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (const auto& n : nodes_) {
        minX = qMin(minX, n.pos.x() - n.radius);
        minY = qMin(minY, n.pos.y() - n.radius);
        maxX = qMax(maxX, n.pos.x() + n.radius);
        maxY = qMax(maxY, n.pos.y() + n.radius);
    }
    qreal w = maxX - minX + 80;
    qreal h = maxY - minY + 80;
    qreal cw = canvas_->width();
    qreal ch = canvas_->height();
    zoom_ = qMin(cw / w, ch / h);
    zoom_ = qBound(0.25, zoom_, 3.0);
    zoomSlider_->blockSignals(true);
    zoomSlider_->setValue(static_cast<int>(zoom_ * 100));
    zoomSlider_->blockSignals(false);
    offset_ = QPointF(cw / 2 - (minX + w / 2) * zoom_, ch / 2 - (minY + h / 2) * zoom_);
    canvas_->update();
}

void PaperMindMapWidget::paintEvent(QPaintEvent*) {
    QPainter p(canvas_);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(canvas_->rect(), QColor(250, 250, 252));

    p.save();
    p.translate(offset_);
    p.scale(zoom_, zoom_);

    for (const auto& edge : edges_)
        drawEdge(p, edge);
    for (const auto& node : nodes_)
        drawNode(p, node);

    p.restore();
}

void PaperMindMapWidget::drawNode(QPainter& p, const MindMapNode& node) {
    p.setPen(Qt::NoPen);
    p.setBrush(node.color.lighter(140));
    p.drawEllipse(node.pos, node.radius + 4, node.radius + 4);

    p.setBrush(node.color);
    p.drawEllipse(node.pos, node.radius, node.radius);

    p.setPen(Qt::white);
    QFont font = p.font();
    font.setPixelSize(qMax(8, static_cast<int>(node.radius * 0.6)));
    font.setBold(true);
    p.setFont(font);
    p.drawText(QRectF(node.pos.x() - node.radius, node.pos.y() - node.radius,
                       node.radius * 2, node.radius * 2),
               Qt::AlignCenter, node.label.left(3));
}

void PaperMindMapWidget::drawEdge(QPainter& p, const MindMapEdge& edge) {
    if (!nodes_.contains(edge.from) || !nodes_.contains(edge.to)) return;
    const auto& a = nodes_[edge.from];
    const auto& b = nodes_[edge.to];

    QPen pen(edge.color, 1.5);
    pen.setStyle(Qt::DashLine);
    p.setPen(pen);
    p.drawLine(a.pos, b.pos);

    if (!edge.label.isEmpty()) {
        QPointF mid = (a.pos + b.pos) / 2;
        p.setPen(edge.color.darker(120));
        QFont font = p.font();
        font.setPixelSize(9);
        p.setFont(font);
        p.drawText(mid, edge.label);
    }
}

void PaperMindMapWidget::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = (event->pos() - offset_) / zoom_;
    int hit = hitTest(scenePos);
    if (hit >= 0) {
        dragNode_ = hit;
        emit nodeClicked(nodes_[hit].paperId);
    } else {
        panning_ = true;
    }
    lastMousePos_ = event->pos();
}

void PaperMindMapWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    QPointF scenePos = (event->pos() - offset_) / zoom_;
    int hit = hitTest(scenePos);
    if (hit >= 0) emit nodeDoubleClicked(nodes_[hit].paperId);
}

void PaperMindMapWidget::mouseMoveEvent(QMouseEvent* event) {
    if (dragNode_ >= 0 && nodes_.contains(dragNode_)) {
        QPointF scenePos = (event->pos() - offset_) / zoom_;
        nodes_[dragNode_].pos = scenePos;
        canvas_->update();
    } else if (panning_) {
        QPointF delta = event->pos() - lastMousePos_;
        offset_ += delta;
        canvas_->update();
    }
    lastMousePos_ = event->pos();
}

void PaperMindMapWidget::wheelEvent(QWheelEvent* event) {
    qreal factor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
    zoom_ = qBound(0.25, zoom_ * factor, 3.0);
    zoomSlider_->blockSignals(true);
    zoomSlider_->setValue(static_cast<int>(zoom_ * 100));
    zoomSlider_->blockSignals(false);
    canvas_->update();
}

int PaperMindMapWidget::hitTest(const QPointF& pos) {
    for (const auto& node : nodes_) {
        qreal dx = pos.x() - node.pos.x();
        qreal dy = pos.y() - node.pos.y();
        if (dx * dx + dy * dy <= node.radius * node.radius)
            return node.id;
    }
    return -1;
}

void PaperMindMapWidget::layoutRadial() {
    if (nodes_.isEmpty()) return;
    int n = nodes_.size();
    int i = 0;
    qreal radius = qMax(80.0, n * 15.0);
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        qreal angle = 2.0 * M_PI * i / n;
        it->pos = QPointF(radius * cos(angle), radius * sin(angle));
        i++;
    }
    refreshOutline();
    canvas_->update();
}

void PaperMindMapWidget::layoutForceDirected() {
    if (nodes_.size() < 2) { layoutRadial(); return; }

    QList<int> ids;
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
        ids.append(it.key());

    for (int iter = 0; iter < 60; ++iter) {
        QMap<int, QPointF> forces;
        for (int id : ids) forces[id] = QPointF(0, 0);

        // Repulsion
        for (int i = 0; i < ids.size(); ++i) {
            for (int j = i + 1; j < ids.size(); ++j) {
                QPointF diff = nodes_[ids[i]].pos - nodes_[ids[j]].pos;
                qreal dist = qMax(1.0, sqrt(diff.x()*diff.x() + diff.y()*diff.y()));
                qreal f = 8000.0 / (dist * dist);
                QPointF unit(diff.x()/dist, diff.y()/dist);
                forces[ids[i]] += unit * f;
                forces[ids[j]] -= unit * f;
            }
        }

        // Attraction (edges)
        for (const auto& edge : edges_) {
            if (!nodes_.contains(edge.from) || !nodes_.contains(edge.to)) continue;
            QPointF diff = nodes_[edge.to].pos - nodes_[edge.from].pos;
            qreal dist = qMax(1.0, sqrt(diff.x()*diff.x() + diff.y()*diff.y()));
            qreal f = (dist - 120.0) * 0.05;
            QPointF unit(diff.x()/dist, diff.y()/dist);
            forces[edge.from] += unit * f;
            forces[edge.to] -= unit * f;
        }

        for (int id : ids) {
            qreal mag = sqrt(forces[id].x()*forces[id].x() + forces[id].y()*forces[id].y());
            qreal maxStep = 5.0;
            if (mag > maxStep) forces[id] *= maxStep / mag;
            nodes_[id].pos += forces[id];
        }
    }
    refreshOutline();
    canvas_->update();
}

void PaperMindMapWidget::layoutTree() {
    if (nodes_.isEmpty()) return;

    int root = nodes_.firstKey();
    QMap<int, int> depth;
    QMap<int, QList<int>> children;
    for (const auto& edge : edges_) {
        if (nodes_.contains(edge.from) && nodes_.contains(edge.to))
            children[edge.from].append(edge.to);
    }

    depth[root] = 0;
    QList<int> queue = {root};
    int maxDepth = 0;
    while (!queue.isEmpty()) {
        int cur = queue.takeFirst();
        for (int child : children[cur]) {
            if (!depth.contains(child)) {
                depth[child] = depth[cur] + 1;
                maxDepth = qMax(maxDepth, depth[child]);
                queue.append(child);
            }
        }
    }

    // Orphan nodes at depth 1
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        if (!depth.contains(it.key())) depth[it.key()] = 1;
    }

    QMap<int, QList<int>> byDepth;
    for (auto it = depth.begin(); it != depth.end(); ++it)
        byDepth[it.value()].append(it.key());

    qreal ySpacing = 100.0;
    for (auto it = byDepth.begin(); it != byDepth.end(); ++it) {
        int d = it.key();
        const auto& level = it.value();
        qreal xSpacing = 600.0 / qMax(1, level.size());
        for (int i = 0; i < level.size(); ++i) {
            if (nodes_.contains(level[i]))
                nodes_[level[i]].pos = QPointF(-300 + xSpacing * (i + 0.5), d * ySpacing);
        }
    }
    refreshOutline();
    canvas_->update();
}

void PaperMindMapWidget::refreshOutline() {
    outlineTree_->clear();
    for (const auto& node : nodes_) {
        auto* item = new QTreeWidgetItem({node.label, QString::number(node.id)});
        item->setForeground(0, node.color);
        outlineTree_->addTopLevelItem(item);
    }
}
