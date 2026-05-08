#include "citation/CitationGraphWidget.hpp"
#include "core/PaperTypes.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>

CitationGraphWidget::CitationGraphWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);
}

void CitationGraphWidget::setPapers(const QList<Paper>& papers) {
    nodes_.clear();

    for (const auto& p : papers) {
        GraphNode node;
        node.paperId = p.id;
        node.title = p.title;
        node.year = p.year;
        node.citations = p.citationCount;
        node.radius = qMax(15.0f, qMin(40.0f, 15.0f + p.citationCount * 0.5f));
        node.pos = QPointF(
            QRandomGenerator::global()->bounded(width() - 100) + 50,
            QRandomGenerator::global()->bounded(height() - 100) + 50
        );
        node.velocity = QPointF(0, 0);
        node.isFocus = (p.id == focusPaperId_);
        nodes_[p.id] = node;
    }

    // Build edges from references
    for (const auto& p : papers) {
        if (!p.keywords.isEmpty() && nodes_.contains(p.id)) {
            // Connect to papers with similar keywords
            for (const auto& other : papers) {
                if (other.id == p.id) continue;
                if (!nodes_.contains(other.id)) continue;

                // Simple similarity: shared keywords
                int shared = 0;
                for (const auto& kw : p.keywords) {
                    if (other.keywords.contains(kw)) shared++;
                }
                if (shared > 0 && !nodes_[p.id].edges.contains(other.id)) {
                    nodes_[p.id].edges.append(other.id);
                }
            }
        }
    }

    layoutNodes();
    update();
}

void CitationGraphWidget::setFocusPaper(int paperId) {
    focusPaperId_ = paperId;
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        it->isFocus = (it->paperId == paperId);
        it->radius = it->isFocus ? 35.0f : qMax(15.0f, qMin(25.0f, 15.0f + it->citations * 0.3f));
    }
    update();
}

void CitationGraphWidget::clear() {
    nodes_.clear();
    update();
}

void CitationGraphWidget::layoutNodes() {
    if (nodes_.isEmpty()) return;
    forceDirectedLayout(100);
}

void CitationGraphWidget::forceDirectedLayout(int iterations) {
    float w = width();
    float h = height();

    for (int iter = 0; iter < iterations; ++iter) {
        // Repulsion between all nodes
        for (auto it1 = nodes_.begin(); it1 != nodes_.end(); ++it1) {
            QPointF force(0, 0);
            for (auto it2 = nodes_.begin(); it2 != nodes_.end(); ++it2) {
                if (it1 == it2) continue;
                QPointF diff = it1->pos - it2->pos;
                float dist = qMax(1.0f, qSqrt(diff.x() * diff.x() + diff.y() * diff.y()));
                float repulsion = 5000.0f / (dist * dist);
                force += diff * (repulsion / dist);
            }

            // Attraction along edges
            for (int targetId : it1->edges) {
                if (!nodes_.contains(targetId)) continue;
                QPointF diff = nodes_[targetId].pos - it1->pos;
                float dist = qSqrt(diff.x() * diff.x() + diff.y() * diff.y());
                float attraction = dist * 0.01f;
                force += diff * (attraction / qMax(1.0f, dist));
            }

            // Center gravity
            QPointF center(w / 2, h / 2);
            QPointF toCenter = center - it1->pos;
            force += toCenter * 0.001f;

            it1->velocity = (it1->velocity + force) * 0.5f;
        }

        for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
            it->pos += it->velocity;
            it->pos.setX(qMax(30.0, qMin(w - 30.0, it->pos.x())));
            it->pos.setY(qMax(30.0, qMin(h - 30.0, it->pos.y())));
        }
    }
}

void CitationGraphWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), palette().window());

    painter.save();
    painter.translate(offset_);
    painter.scale(zoom_, zoom_);

    // Draw edges
    QPen edgePen(QColor(200, 200, 200), 1.5);
    painter.setPen(edgePen);
    for (auto it = nodes_.constBegin(); it != nodes_.constEnd(); ++it) {
        for (int targetId : it->edges) {
            if (!nodes_.contains(targetId)) continue;
            const auto& target = nodes_[targetId];
            painter.drawLine(it->pos, target.pos);
        }
    }

    // Draw nodes
    for (auto it = nodes_.constBegin(); it != nodes_.constEnd(); ++it) {
        const auto& node = it.value();

        // Glow for focus node
        if (node.isFocus) {
            QRadialGradient gradient(node.pos, node.radius * 2);
            gradient.setColorAt(0, QColor(59, 130, 246, 100));
            gradient.setColorAt(1, QColor(59, 130, 246, 0));
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(node.pos, node.radius * 2, node.radius * 2);
        }

        // Node circle
        QColor nodeColor = node.isFocus ? QColor(59, 130, 246) : QColor(107, 114, 128);
        painter.setBrush(nodeColor);
        painter.setPen(QPen(nodeColor.darker(120), 2));
        painter.drawEllipse(node.pos, node.radius, node.radius);

        // Title
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        QRectF textRect(node.pos.x() - node.radius, node.pos.y() - node.radius,
                         node.radius * 2, node.radius * 2);
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap,
                         node.title.left(15));

        // Year below
        painter.setPen(palette().mid().color());
        painter.setFont(QFont("Arial", 7));
        painter.drawText(QRectF(node.pos.x() - 30, node.pos.y() + node.radius + 2, 60, 14),
                         Qt::AlignCenter, node.year);
    }

    painter.restore();

    // Legend
    painter.setPen(palette().mid().color());
    painter.setFont(QFont("Arial", 9));
    painter.drawText(10, height() - 10, "Scroll: zoom | Drag: move | Click: select paper");
}

void CitationGraphWidget::mousePressEvent(QMouseEvent* event) {
    QPointF pos = (event->pos() - offset_) / zoom_;

    for (auto it = nodes_.constBegin(); it != nodes_.constEnd(); ++it) {
        QPointF diff = pos - it->pos;
        if (diff.x() * diff.x() + diff.y() * diff.y() < it->radius * it->radius) {
            emit paperClicked(it->paperId);
            setFocusPaper(it->paperId);
            return;
        }
    }

    draggingNode_ = -1;
    lastMousePos_ = event->pos();
}

void CitationGraphWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        offset_ += event->pos() - lastMousePos_;
        lastMousePos_ = event->pos();
        update();
    }
}

void CitationGraphWidget::wheelEvent(QWheelEvent* event) {
    float delta = event->angleDelta().y() > 0 ? 1.1f : 0.9f;
    zoom_ *= delta;
    zoom_ = qMax(0.3f, qMin(3.0f, zoom_));
    update();
}
