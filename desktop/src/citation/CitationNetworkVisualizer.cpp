#include "citation/CitationNetworkVisualizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <cmath>

CitationNetworkVisualizer::CitationNetworkVisualizer(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void CitationNetworkVisualizer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Layout:"));
    layoutCombo_ = new QComboBox();
    layoutCombo_->addItems({"Force-Directed", "Circular", "Hierarchical", "Random"});
    connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CitationNetworkVisualizer::onLayoutChanged);
    toolbar->addWidget(layoutCombo_);

    fitBtn_ = new QPushButton("Fit");
    connect(fitBtn_, &QPushButton::clicked, this, &CitationNetworkVisualizer::onFitView);
    toolbar->addWidget(fitBtn_);

    resetBtn_ = new QPushButton("Reset");
    connect(resetBtn_, &QPushButton::clicked, this, &CitationNetworkVisualizer::onResetView);
    toolbar->addWidget(resetBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);

    auto* canvas = new QWidget();
    canvas->setMinimumSize(400, 300);
    canvas->setMouseTracking(true);
    splitter->addWidget(canvas);

    infoList_ = new QListWidget();
    infoList_->setMaximumWidth(180);
    infoList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 2px; }"
    );
    splitter->addWidget(infoList_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("Load papers to visualize citation network");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void CitationNetworkVisualizer::setPapers(const QList<QPair<int, QString>>& papers) {
    nodes_.clear();
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153),
                       QColor(14,165,233), QColor(168,85,247)};
    for (int i = 0; i < papers.size(); ++i) {
        CitationNode node;
        node.paperId = papers[i].first;
        node.title = papers[i].second;
        node.pos = QPointF(
            QRandomGenerator::global()->bounded(300.0) - 150,
            QRandomGenerator::global()->bounded(300.0) - 150
        );
        node.color = colors[i % 8];
        node.radius = 15 + (i % 3) * 5;
        node.citationCount = QRandomGenerator::global()->bounded(50);
        nodes_.append(node);
    }

    // Generate some random citations
    for (int i = 0; i < qMin(nodes_.size() * 2, 40); ++i) {
        int from = QRandomGenerator::global()->bounded(nodes_.size());
        int to = QRandomGenerator::global()->bounded(nodes_.size());
        if (from != to) {
            addCitation(nodes_[from].paperId, nodes_[to].paperId);
        }
    }

    computeLayout();
    statsLabel_->setText(QString("%1 nodes, %2 edges").arg(nodes_.size()).arg(edges_.size()));
    update();
}

void CitationNetworkVisualizer::addCitation(int fromId, int toId, const QString& type) {
    CitationEdge edge;
    edge.fromId = fromId;
    edge.toId = toId;
    edge.type = type;
    edges_.append(edge);
}

void CitationNetworkVisualizer::clear() {
    nodes_.clear();
    edges_.clear();
    infoList_->clear();
    update();
}

void CitationNetworkVisualizer::computeLayout() {
    if (nodes_.isEmpty()) return;
    QString method = layoutCombo_->currentText();

    if (method == "Random") {
        for (auto& n : nodes_) {
            n.pos = QPointF(QRandomGenerator::global()->bounded(300.0) - 150,
                            QRandomGenerator::global()->bounded(300.0) - 150);
        }
    } else if (method == "Circular") {
        qreal radius = 120;
        qreal angle = 0;
        qreal step = 2.0 * M_PI / nodes_.size();
        for (auto& n : nodes_) {
            n.pos = QPointF(radius * std::cos(angle), radius * std::sin(angle));
            angle += step;
        }
    } else if (method == "Hierarchical") {
        // Layer by citation count
        int maxCite = 1;
        for (const auto& n : nodes_) maxCite = qMax(maxCite, n.citationCount + 1);
        QMap<int, QList<CitationNode*>> layers;
        for (auto& n : nodes_) {
            int layer = qBound(0, (n.citationCount * 4) / maxCite, 3);
            layers[layer].append(&n);
        }
        for (auto it = layers.begin(); it != layers.end(); ++it) {
            int layer = it.key();
            auto& items = it.value();
            qreal y = -150 + layer * 100;
            qreal xStart = -(items.size() - 1) * 40.0;
            for (int i = 0; i < items.size(); ++i) {
                items[i]->pos = QPointF(xStart + i * 80, y);
            }
        }
    } else {
        // Force-directed (simplified)
        for (auto& n : nodes_) {
            n.pos = QPointF(QRandomGenerator::global()->bounded(200.0) - 100,
                            QRandomGenerator::global()->bounded(200.0) - 100);
        }
        for (int iter = 0; iter < 50; ++iter) {
            for (int i = 0; i < nodes_.size(); ++i) {
                QPointF force(0, 0);
                for (int j = 0; j < nodes_.size(); ++j) {
                    if (i == j) continue;
                    QPointF diff = nodes_[i].pos - nodes_[j].pos;
                    qreal dist = qMax(1.0, std::sqrt(diff.x() * diff.x() + diff.y() * diff.y()));
                    force += diff * (200.0 / (dist * dist));
                }
                for (const auto& e : edges_) {
                    int other = -1;
                    if (e.fromId == nodes_[i].paperId) other = e.toId;
                    else if (e.toId == nodes_[i].paperId) other = e.fromId;
                    if (other < 0) continue;
                    for (auto& n : nodes_) {
                        if (n.paperId == other) {
                            QPointF diff = n.pos - nodes_[i].pos;
                            qreal dist = qMax(1.0, std::sqrt(diff.x() * diff.x() + diff.y() * diff.y()));
                            force += diff * (dist - 80) * 0.01;
                            break;
                        }
                    }
                }
                nodes_[i].pos += force * 0.1;
            }
        }
    }
    update();
}

void CitationNetworkVisualizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(15, 23, 42));

    p.save();
    p.translate(offset_);
    p.scale(zoom_, zoom_);

    // Edges
    p.setPen(QPen(QColor(100, 116, 139, 120), 1.5));
    for (const auto& e : edges_) {
        QPointF from, to;
        bool foundFrom = false, foundTo = false;
        for (const auto& n : nodes_) {
            if (n.paperId == e.fromId) { from = n.pos; foundFrom = true; }
            if (n.paperId == e.toId) { to = n.pos; foundTo = true; }
        }
        if (foundFrom && foundTo) {
            p.drawLine(from, to);
            // Arrow head
            QPointF diff = to - from;
            qreal len = std::sqrt(diff.x() * diff.x() + diff.y() * diff.y());
            if (len > 0) {
                QPointF mid = from + diff * 0.7;
                QPointF perp(-diff.y() / len * 5, diff.x() / len * 5);
                p.setBrush(QColor(100, 116, 139, 120));
                QPolygonF arrow;
                arrow << (mid + diff / len * 6) << (mid + perp) << (mid - perp);
                p.drawPolygon(arrow);
                p.setBrush(Qt::NoBrush);
            }
        }
    }

    // Nodes
    for (const auto& n : nodes_) {
        p.setPen(Qt::NoPen);
        p.setBrush(n.color.lighter(150));
        p.drawEllipse(n.pos, n.radius + 2, n.radius + 2);
        p.setBrush(n.color);
        p.drawEllipse(n.pos, n.radius, n.radius);

        if (n.citationCount > 0) {
            p.setPen(QPen(Qt::white, 1));
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(QRectF(n.pos.x() - n.radius, n.pos.y() - n.radius,
                              n.radius * 2, n.radius * 2),
                       Qt::AlignCenter, QString::number(n.citationCount));
        }
    }

    p.restore();
}

void CitationNetworkVisualizer::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = (event->pos() - offset_) / zoom_;
    int hit = hitTest(scenePos);
    if (hit >= 0) {
        emit nodeClicked(hit);
        for (const auto& n : nodes_) {
            if (n.paperId == hit) {
                statsLabel_->setText(QString("Paper #%1: %2 (%3 cites)")
                    .arg(hit).arg(n.title.left(50)).arg(n.citationCount));
                break;
            }
        }
    } else {
        panning_ = true;
    }
    lastMousePos_ = event->pos();
}

void CitationNetworkVisualizer::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        offset_ += event->pos() - lastMousePos_;
        update();
    }
    lastMousePos_ = event->pos();
}

void CitationNetworkVisualizer::wheelEvent(QWheelEvent* event) {
    qreal factor = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
    zoom_ = qBound(0.1, zoom_ * factor, 5.0);
    update();
}

int CitationNetworkVisualizer::hitTest(const QPointF& pos) {
    for (const auto& n : nodes_) {
        qreal dx = pos.x() - n.pos.x();
        qreal dy = pos.y() - n.pos.y();
        if (dx * dx + dy * dy <= n.radius * n.radius) return n.paperId;
    }
    return -1;
}

void CitationNetworkVisualizer::onLayoutChanged(int) { computeLayout(); }

void CitationNetworkVisualizer::onFitView() {
    if (nodes_.isEmpty()) return;
    qreal minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (const auto& n : nodes_) {
        minX = qMin(minX, n.pos.x() - n.radius);
        minY = qMin(minY, n.pos.y() - n.radius);
        maxX = qMax(maxX, n.pos.x() + n.radius);
        maxY = qMax(maxY, n.pos.y() + n.radius);
    }
    qreal w = maxX - minX + 40, h = maxY - minY + 40;
    zoom_ = qMin(width() * 0.7 / w, height() * 0.7 / h);
    zoom_ = qBound(0.1, zoom_, 5.0);
    offset_ = QPointF(width() / 2 - (minX + w / 2) * zoom_, height() / 2 - (minY + h / 2) * zoom_);
    update();
}

void CitationNetworkVisualizer::onResetView() {
    offset_ = QPointF(0, 0);
    zoom_ = 1.0;
    update();
}
