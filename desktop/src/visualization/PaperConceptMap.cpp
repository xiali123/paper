#include "visualization/PaperConceptMap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QRandomGenerator>
#include <cmath>

PaperConceptMap::PaperConceptMap(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperConceptMap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Extract or add concepts");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Layout:"));
    layoutCombo_ = new QComboBox();
    layoutCombo_->addItems({"Force", "Circular", "Grouped", "Random"});
    connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperConceptMap::onLayoutChanged);
    toolbar->addWidget(layoutCombo_);

    fitBtn_ = new QPushButton("Fit");
    connect(fitBtn_, &QPushButton::clicked, this, &PaperConceptMap::onFitView);
    toolbar->addWidget(fitBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);
    auto* canvas = new QWidget();
    canvas->setMinimumSize(400, 300);
    canvas->setMouseTracking(true);
    splitter->addWidget(canvas);

    conceptList_ = new QListWidget();
    conceptList_->setMaximumWidth(170);
    conceptList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 2px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(conceptList_, &QListWidget::itemClicked, this, &PaperConceptMap::onConceptListClicked);
    splitter->addWidget(conceptList_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 concepts");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperConceptMap::setPaper(int paperId, const QString& title) {
    Q_UNUSED(paperId);
    paperLabel_->setText(QString("Concepts: %1").arg(title));
}

void PaperConceptMap::addConcept(const ConceptNode& node) {
    for (auto& c : concepts_) {
        if (c.label == node.label) {
            c.frequency++;
            update();
            return;
        }
    }
    ConceptNode n = node;
    if (n.id < 0) n.id = nextId_++;
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153)};
    if (!n.color.isValid()) n.color = colors[n.id % 6];
    n.pos = QPointF(QRandomGenerator::global()->bounded(200.0) - 100,
                    QRandomGenerator::global()->bounded(200.0) - 100);
    concepts_.append(n);

    conceptList_->clear();
    for (const auto& c : concepts_) {
        auto* item = new QListWidgetItem(QString("%1 (%2)").arg(c.label).arg(c.frequency));
        item->setData(Qt::UserRole, c.id);
        item->setForeground(c.color);
        conceptList_->addItem(item);
    }

    statsLabel_->setText(QString("%1 concepts, %2 connections").arg(concepts_.size()).arg(edges_.size()));
    emit conceptAdded(n.id);
    update();
}

void PaperConceptMap::addConnection(int fromId, int toId, qreal weight) {
    ConceptEdge e;
    e.fromId = fromId;
    e.toId = toId;
    e.weight = weight;
    edges_.append(e);
}

void PaperConceptMap::extractFromText(const QString& text) {
    QStringList stopwords = {"the","a","an","is","are","was","were","be","been","being",
        "have","has","had","do","does","did","will","would","could","should","may","might",
        "can","shall","to","of","in","for","on","with","at","by","from","as","into","through",
        "during","before","after","above","below","between","out","off","over","under","again",
        "further","then","once","here","there","when","where","why","how","all","both","each",
        "few","more","most","other","some","such","no","not","only","own","same","so","than",
        "too","very","just","because","but","and","or","if","while","about","up","it","its",
        "this","that","these","those","we","our","they","their","which","what","who","i","my"};

    QMap<QString, int> freq;
    QStringList words = text.toLower().split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    for (const auto& w : words) {
        if (w.length() < 3 || stopwords.contains(w)) continue;
        freq[w]++;
    }

    QList<QPair<QString, int>> sorted;
    for (auto it = freq.begin(); it != freq.end(); ++it) {
        sorted.append({it.key(), it.value()});
    }
    std::sort(sorted.begin(), sorted.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    int count = qMin(20, sorted.size());
    for (int i = 0; i < count; ++i) {
        ConceptNode n;
        n.label = sorted[i].first;
        n.label[0] = n.label[0].toUpper();
        n.frequency = sorted[i].second;
        n.radius = 12 + qMin(sorted[i].second * 2, 20);
        addConcept(n);
    }

    // Connect co-occurring concepts
    for (int i = 0; i < concepts_.size(); ++i) {
        for (int j = i + 1; j < qMin(i + 4, concepts_.size()); ++j) {
            addConnection(concepts_[i].id, concepts_[j].id, 0.5);
        }
    }

    onLayoutChanged(0);
}

void PaperConceptMap::clear() {
    concepts_.clear();
    edges_.clear();
    conceptList_->clear();
    nextId_ = 1;
    update();
}

QList<ConceptNode> PaperConceptMap::concepts() const { return concepts_; }

void PaperConceptMap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(15, 23, 42));

    p.save();
    p.translate(offset_);
    p.scale(zoom_, zoom_);

    // Edges
    for (const auto& e : edges_) {
        QPointF from, to;
        bool f1 = false, f2 = false;
        for (const auto& c : concepts_) {
            if (c.id == e.fromId) { from = c.pos; f1 = true; }
            if (c.id == e.toId) { to = c.pos; f2 = true; }
        }
        if (f1 && f2) {
            QColor edgeColor = (selectedId_ == e.fromId || selectedId_ == e.toId)
                ? QColor(59, 130, 246, 180) : QColor(100, 116, 139, 60);
            p.setPen(QPen(edgeColor, e.weight * 2));
            p.drawLine(from, to);
        }
    }

    // Nodes
    for (const auto& c : concepts_) {
        bool sel = (c.id == selectedId_);
        p.setPen(Qt::NoPen);
        if (sel) {
            p.setBrush(c.color.lighter(140));
            p.drawEllipse(c.pos, c.radius + 3, c.radius + 3);
        }
        p.setBrush(c.color);
        p.drawEllipse(c.pos, c.radius, c.radius);

        p.setPen(QPen(Qt::white, 1));
        p.setFont(QFont("Arial", qMax(7, qMin(10, static_cast<int>(c.radius * 0.5)))));
        p.drawText(QRectF(c.pos.x() - c.radius, c.pos.y() - c.radius,
                          c.radius * 2, c.radius * 2),
                   Qt::AlignCenter, c.label);
    }

    p.restore();
}

void PaperConceptMap::mousePressEvent(QMouseEvent* event) {
    QPointF scenePos = (event->pos() - offset_) / zoom_;
    int hit = hitTest(scenePos);
    if (hit >= 0) {
        selectedId_ = hit;
        for (const auto& c : concepts_) {
            if (c.id == hit) {
                statsLabel_->setText(QString("%1 (freq: %2)").arg(c.label).arg(c.frequency));
                emit conceptClicked(hit, c.label);
                break;
            }
        }
        update();
    } else {
        panning_ = true;
    }
    lastMousePos_ = event->pos();
}

void PaperConceptMap::mouseMoveEvent(QMouseEvent* event) {
    if (panning_) {
        offset_ += event->pos() - lastMousePos_;
        update();
    }
    lastMousePos_ = event->pos();
}

void PaperConceptMap::wheelEvent(QWheelEvent* event) {
    qreal factor = (event->angleDelta().y() > 0) ? 1.15 : 0.87;
    zoom_ = qBound(0.1, zoom_ * factor, 5.0);
    update();
}

int PaperConceptMap::hitTest(const QPointF& pos) {
    for (const auto& c : concepts_) {
        qreal dx = pos.x() - c.pos.x();
        qreal dy = pos.y() - c.pos.y();
        if (dx * dx + dy * dy <= c.radius * c.radius) return c.id;
    }
    return -1;
}

void PaperConceptMap::forceDirectedLayout() {
    for (auto& c : concepts_) {
        c.pos = QPointF(QRandomGenerator::global()->bounded(200.0) - 100,
                        QRandomGenerator::global()->bounded(200.0) - 100);
    }
    for (int iter = 0; iter < 50; ++iter) {
        for (int i = 0; i < concepts_.size(); ++i) {
            QPointF force(0, 0);
            for (int j = 0; j < concepts_.size(); ++j) {
                if (i == j) continue;
                QPointF diff = concepts_[i].pos - concepts_[j].pos;
                qreal dist = qMax(1.0, std::sqrt(diff.x() * diff.x() + diff.y() * diff.y()));
                force += diff * (250.0 / (dist * dist));
            }
            for (const auto& e : edges_) {
                int other = -1;
                if (e.fromId == concepts_[i].id) other = e.toId;
                else if (e.toId == concepts_[i].id) other = e.fromId;
                if (other < 0) continue;
                for (auto& n : concepts_) {
                    if (n.id == other) {
                        QPointF diff = n.pos - concepts_[i].pos;
                        qreal dist = qMax(1.0, std::sqrt(diff.x() * diff.x() + diff.y() * diff.y()));
                        force += diff * (dist - 60) * 0.01;
                        break;
                    }
                }
            }
            concepts_[i].pos += force * 0.08;
        }
    }
    update();
}

void PaperConceptMap::circularLayout() {
    qreal radius = 80 + concepts_.size() * 5;
    qreal angle = 0;
    qreal step = 2.0 * M_PI / qMax(1, concepts_.size());
    for (auto& c : concepts_) {
        c.pos = QPointF(radius * std::cos(angle), radius * std::sin(angle));
        angle += step;
    }
    update();
}

void PaperConceptMap::onLayoutChanged(int) {
    QString method = layoutCombo_->currentText();
    if (method == "Force") forceDirectedLayout();
    else if (method == "Circular") circularLayout();
    else if (method == "Grouped") {
        QMap<QString, QList<ConceptNode*>> groups;
        for (auto& c : concepts_) groups[c.category.isEmpty() ? "default" : c.category].append(&c);
        qreal y = -100;
        for (auto it = groups.begin(); it != groups.end(); ++it) {
            qreal x = -80;
            for (auto* n : it.value()) {
                n->pos = QPointF(x, y);
                x += 60;
            }
            y += 70;
        }
        update();
    } else {
        for (auto& c : concepts_) {
            c.pos = QPointF(QRandomGenerator::global()->bounded(300.0) - 150,
                            QRandomGenerator::global()->bounded(300.0) - 150);
        }
        update();
    }
}

void PaperConceptMap::onFitView() {
    if (concepts_.isEmpty()) return;
    qreal minX = 1e9, minY = 1e9, maxX = -1e9, maxY = -1e9;
    for (const auto& c : concepts_) {
        minX = qMin(minX, c.pos.x() - c.radius);
        minY = qMin(minY, c.pos.y() - c.radius);
        maxX = qMax(maxX, c.pos.x() + c.radius);
        maxY = qMax(maxY, c.pos.y() + c.radius);
    }
    qreal w = maxX - minX + 60, h = maxY - minY + 60;
    zoom_ = qMin(width() * 0.7 / w, height() * 0.7 / h);
    zoom_ = qBound(0.1, zoom_, 5.0);
    offset_ = QPointF(width() / 2 - (minX + w / 2) * zoom_, height() / 2 - (minY + h / 2) * zoom_);
    update();
}

void PaperConceptMap::onConceptListClicked() {
    auto* item = conceptList_->currentItem();
    if (!item) return;
    selectedId_ = item->data(Qt::UserRole).toInt();
    update();
}
