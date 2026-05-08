#include "PaperEmbeddingWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <cmath>

PaperEmbeddingWidget::PaperEmbeddingWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperEmbeddingWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Method:"));
    methodCombo_ = new QComboBox();
    methodCombo_->addItems({"t-SNE", "PCA", "UMAP", "Random"});
    connect(methodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperEmbeddingWidget::onMethodChanged);
    toolbar->addWidget(methodCombo_);

    toolbar->addWidget(new QLabel("Color:"));
    colorByCombo_ = new QComboBox();
    colorByCombo_->addItems({"Cluster", "Year", "Index"});
    connect(colorByCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperEmbeddingWidget::onColorByChanged);
    toolbar->addWidget(colorByCombo_);

    recomputeBtn_ = new QPushButton("Recompute");
    recomputeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(recomputeBtn_, &QPushButton::clicked, this, &PaperEmbeddingWidget::onRecompute);
    toolbar->addWidget(recomputeBtn_);

    fitBtn_ = new QPushButton("Fit");
    connect(fitBtn_, &QPushButton::clicked, this, &PaperEmbeddingWidget::onFitView);
    toolbar->addWidget(fitBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);

    auto* canvas = new QWidget();
    canvas->setMinimumSize(400, 300);
    canvas->setMouseTracking(true);
    splitter->addWidget(canvas);

    legendList_ = new QListWidget();
    legendList_->setMaximumWidth(160);
    legendList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 2px; }"
    );
    splitter->addWidget(legendList_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("Load papers to visualize embeddings");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperEmbeddingWidget::setPapers(const QList<QPair<int, QString>>& papers) {
    points_.clear();
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153),
                       QColor(14,165,233), QColor(168,85,247)};
    for (int i = 0; i < papers.size(); ++i) {
        EmbeddingPoint pt;
        pt.paperId = papers[i].first;
        pt.title = papers[i].second;
        pt.pos = QPointF(
            QRandomGenerator::global()->bounded(200.0) - 100,
            QRandomGenerator::global()->bounded(200.0) - 100
        );
        pt.color = colors[i % 8];
        pt.cluster = QString("Cluster %1").arg(i % 5);
        points_.append(pt);
    }
    statsLabel_->setText(QString("%1 papers loaded").arg(points_.size()));
    update();
}

void PaperEmbeddingWidget::setEmbeddings(const QMap<int, QPointF>& embeddings) {
    for (auto& pt : points_) {
        if (embeddings.contains(pt.paperId)) {
            pt.pos = embeddings[pt.paperId];
        }
    }
    update();
}

void PaperEmbeddingWidget::computeRandomEmbeddings() {
    // Simulate t-SNE-like layout with cluster structure
    QMap<QString, QPointF> clusterCenters;
    QList<QString> clusterNames;
    for (const auto& pt : points_) {
        if (!clusterCenters.contains(pt.cluster)) {
            clusterCenters[pt.cluster] = QPointF(
                QRandomGenerator::global()->bounded(300.0) - 150,
                QRandomGenerator::global()->bounded(300.0) - 150
            );
            clusterNames.append(pt.cluster);
        }
    }
    for (auto& pt : points_) {
        QPointF center = clusterCenters[pt.cluster];
        pt.pos = center + QPointF(
            QRandomGenerator::global()->bounded(60.0) - 30,
            QRandomGenerator::global()->bounded(60.0) - 30
        );
    }
    update();
    statsLabel_->setText(QString("%1 points, %2 clusters").arg(points_.size()).arg(clusterNames.size()));

    legendList_->clear();
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153),
                       QColor(14,165,233), QColor(168,85,247)};
    for (int i = 0; i < clusterNames.size(); ++i) {
        auto* item = new QListWidgetItem(clusterNames[i]);
        item->setForeground(colors[i % 8]);
        legendList_->addItem(item);
    }
}

void PaperEmbeddingWidget::colorByCluster(const QMap<int, QString>& clusters) {
    QMap<QString, QColor> colorMap;
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153),
                       QColor(14,165,233), QColor(168,85,247)};
    int ci = 0;
    for (auto it = clusters.begin(); it != clusters.end(); ++it) {
        if (!colorMap.contains(it.value())) {
            colorMap[it.value()] = colors[ci % 8];
            ci++;
        }
    }
    for (auto& pt : points_) {
        pt.cluster = clusters.value(pt.paperId, "Unknown");
        pt.color = colorMap.value(pt.cluster, QColor(128, 128, 128));
    }
    update();
}

void PaperEmbeddingWidget::colorByYear(const QMap<int, int>& years) {
    int minYear = 9999, maxYear = 0;
    for (const auto& y : years) { minYear = qMin(minYear, y); maxYear = qMax(maxYear, y); }
    int range = qMax(1, maxYear - minYear);
    for (auto& pt : points_) {
        int year = years.value(pt.paperId, minYear);
        qreal t = static_cast<qreal>(year - minYear) / range;
        pt.color = QColor::fromHsvF(t * 0.7, 0.8, 0.9);
        pt.cluster = QString::number(year);
    }
    update();
}

void PaperEmbeddingWidget::clear() {
    points_.clear();
    legendList_->clear();
    update();
}

void PaperEmbeddingWidget::onMethodChanged(int) { onRecompute(); }
void PaperEmbeddingWidget::onFitView() {
    if (points_.isEmpty()) return;
    qreal minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (const auto& pt : points_) {
        minX = qMin(minX, pt.pos.x()); minY = qMin(minY, pt.pos.y());
        maxX = qMax(maxX, pt.pos.x()); maxY = qMax(maxY, pt.pos.y());
    }
    qreal w = maxX - minX + 40, h = maxY - minY + 40;
    zoom_ = qMin(width() * 0.7 / w, height() * 0.7 / h);
    zoom_ = qBound(0.1, zoom_, 5.0);
    offset_ = QPointF(width() / 2 - (minX + w / 2) * zoom_, height() / 2 - (minY + h / 2) * zoom_);
    update();
}

void PaperEmbeddingWidget::onColorByChanged(int index) {
    Q_UNUSED(index);
    // Recolor based on existing data
    update();
}

void PaperEmbeddingWidget::onRecompute() {
    computeRandomEmbeddings();
    emit recomputeRequested(methodCombo_->currentText());
}

void PaperEmbeddingWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(15, 23, 42));

    p.save();
    p.translate(offset_);
    p.scale(zoom_, zoom_);

    // Grid lines
    p.setPen(QPen(QColor(30, 41, 59), 0.5));
    for (int i = -200; i <= 200; i += 50) {
        p.drawLine(i, -200, i, 200);
        p.drawLine(-200, i, 200, i);
    }

    // Points
    for (const auto& pt : points_) {
        p.setPen(Qt::NoPen);
        p.setBrush(pt.color.lighter(150));
        p.drawEllipse(pt.pos, 8, 8);
        p.setBrush(pt.color);
        p.drawEllipse(pt.pos, 6, 6);
    }

    // Labels on hover
    p.restore();
}

void PaperEmbeddingWidget::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = (event->pos() - offset_) / zoom_;
    int hit = hitTest(scenePos);
    if (hit >= 0) {
        for (const auto& pt : points_) {
            if (pt.paperId == hit) {
                emit pointClicked(hit);
                statsLabel_->setText(QString("Paper #%1: %2").arg(hit).arg(pt.title.left(60)));
                break;
            }
        }
    } else {
        panning_ = true;
    }
    lastMousePos_ = event->pos();
}

void PaperEmbeddingWidget::wheelEvent(QWheelEvent* event) {
    qreal factor = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
    zoom_ = qBound(0.1, zoom_ * factor, 5.0);
    update();
}

void PaperEmbeddingWidget::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        offset_ += event->pos() - lastMousePos_;
        update();
    }
    lastMousePos_ = event->pos();
}

int PaperEmbeddingWidget::hitTest(const QPointF& pos) {
    for (const auto& pt : points_) {
        qreal dx = pos.x() - pt.pos.x();
        qreal dy = pos.y() - pt.pos.y();
        if (dx * dx + dy * dy <= 36) return pt.paperId;
    }
    return -1;
}
