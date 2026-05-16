#include "visualization/PaperChordWheel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QtMath>

PaperChordWheel::PaperChordWheel(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ChordWheel")
{
    setupUI();
    loadSettings();
}

void PaperChordWheel::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citation", "Collaboration", "Reference", "Topic"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Source->Target...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperChordWheel::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperChordWheel::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add chord connections to render wheel");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 600);
}

void PaperChordWheel::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add chord connections to render wheel");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Chord Wheel");

    int w = width(), h = height();
    int halfH = (h - 70) / 2;

    drawChordWheel(p, QRect(20, 45, w - 40, halfH));
    drawCategoryLegend(p, QRect(20, 55 + halfH, w / 2 - 30, halfH - 20));
    drawStats(p, QRect(w / 2 + 10, 55 + halfH, w / 2 - 30, halfH - 20));
}

void PaperChordWheel::drawChordWheel(QPainter& p, const QRect& rect) {
    // Collect unique nodes
    QSet<QString> nodeSet;
    for (const auto& e : entries_) {
        nodeSet.insert(e.source);
        nodeSet.insert(e.target);
    }
    QList<QString> nodes = nodeSet.values();
    int n = nodes.size();
    if (n < 2) return;

    // Category color mapping
    static const QMap<QString, QColor> catColors = {
        {"Citation",      QColor(0x3b, 0x82, 0xf6)},
        {"Collaboration", QColor(0x16, 0xa3, 0x4a)},
        {"Reference",     QColor(0x7c, 0x3a, 0xed)},
        {"Topic",         QColor(0xd9, 0x77, 0x06)}
    };

    // Compute per-node total weight for arc sizing
    QMap<QString, qreal> nodeWeight;
    for (const auto& e : entries_) {
        nodeWeight[e.source] += e.weight;
        nodeWeight[e.target] += e.weight;
    }
    qreal totalNodeWeight = 0;
    for (const auto& node : nodes) totalNodeWeight += nodeWeight[node];
    if (totalNodeWeight <= 0) totalNodeWeight = 1.0;

    // Circle geometry
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 40;
    if (radius < 40) radius = 40;

    // Build arc layout for each node
    struct ArcInfo {
        QString name;
        qreal startAngle;
        qreal spanAngle;
        QColor color;
    };
    QList<ArcInfo> arcs;

    qreal gapAngle = 0.02;
    qreal availableAngle = 2.0 * M_PI - gapAngle * n;
    qreal currentAngle = -M_PI / 2;

    // Determine dominant category color for each node
    QMap<QString, QMap<QString, qreal>> nodeCatWeight;
    for (const auto& e : entries_) {
        nodeCatWeight[e.source][e.category] += e.weight;
        nodeCatWeight[e.target][e.category] += e.weight;
    }

    for (int i = 0; i < n; ++i) {
        qreal proportion = nodeWeight[nodes[i]] / totalNodeWeight;
        qreal span = proportion * availableAngle;

        // Pick dominant category color for this node
        QColor nodeColor(0x94, 0xa3, 0xb8); // fallback slate
        qreal maxCW = 0;
        for (auto it = nodeCatWeight[nodes[i]].cbegin(); it != nodeCatWeight[nodes[i]].cend(); ++it) {
            if (it.value() > maxCW && catColors.contains(it.key())) {
                maxCW = it.value();
                nodeColor = catColors[it.key()];
            }
        }

        ArcInfo arc;
        arc.name = nodes[i];
        arc.startAngle = currentAngle;
        arc.spanAngle = span;
        arc.color = nodeColor;
        arcs.append(arc);
        currentAngle += span + gapAngle;
    }

    QMap<QString, ArcInfo> arcMap;
    for (const auto& arc : arcs) arcMap[arc.name] = arc;

    // Compute max weight for normalization
    qreal maxWeight = 1.0;
    for (const auto& e : entries_) maxWeight = qMax(maxWeight, e.weight);

    // Draw curved chords first (under arcs)
    for (const auto& e : entries_) {
        if (!arcMap.contains(e.source) || !arcMap.contains(e.target)) continue;

        const ArcInfo& srcArc = arcMap[e.source];
        const ArcInfo& tgtArc = arcMap[e.target];

        qreal srcMid = srcArc.startAngle + srcArc.spanAngle / 2.0;
        qreal srcX = cx + radius * qCos(srcMid);
        qreal srcY = cy + radius * qSin(srcMid);

        qreal tgtMid = tgtArc.startAngle + tgtArc.spanAngle / 2.0;
        qreal tgtX = cx + radius * qCos(tgtMid);
        qreal tgtY = cy + radius * qSin(tgtMid);

        // Cubic bezier control points pulled toward center
        qreal pullFactor = 0.3 + 0.4 * (e.weight / maxWeight);
        qreal ctrlSrcX = cx + (srcX - cx) * pullFactor;
        qreal ctrlSrcY = cy + (srcY - cy) * pullFactor;
        qreal ctrlTgtX = cx + (tgtX - cx) * pullFactor;
        qreal ctrlTgtY = cy + (tgtY - cy) * pullFactor;

        // Color and alpha from category
        QColor chordColor = catColors.value(e.category, QColor(0x94, 0xa3, 0xb8));
        int alpha = 40 + static_cast<int>(160 * (e.weight / maxWeight));
        chordColor.setAlpha(alpha);

        QPainterPath path;
        path.moveTo(srcX, srcY);
        path.cubicTo(ctrlSrcX, ctrlSrcY, ctrlTgtX, ctrlTgtY, tgtX, tgtY);

        qreal lineWidth = 1.0 + 4.0 * (e.weight / maxWeight);
        QPen pen(chordColor, lineWidth);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // Bidirectional return chord with slight offset
        if (e.bidirectional) {
            qreal offset = 0.04;
            qreal srcMid2 = srcMid + offset;
            qreal tgtMid2 = tgtMid - offset;
            qreal s2x = cx + radius * qCos(srcMid2);
            qreal s2y = cy + radius * qSin(srcMid2);
            qreal t2x = cx + radius * qCos(tgtMid2);
            qreal t2y = cy + radius * qSin(tgtMid2);
            qreal cs2x = cx + (s2x - cx) * pullFactor;
            qreal cs2y = cy + (s2y - cy) * pullFactor;
            qreal ct2x = cx + (t2x - cx) * pullFactor;
            qreal ct2y = cy + (t2y - cy) * pullFactor;

            QColor retColor = chordColor.lighter(120);
            retColor.setAlpha(alpha);
            QPainterPath retPath;
            retPath.moveTo(t2x, t2y);
            retPath.cubicTo(ct2x, ct2y, cs2x, cs2y, s2x, s2y);
            QPen retPen(retColor, lineWidth * 0.7);
            retPen.setCapStyle(Qt::RoundCap);
            p.setPen(retPen);
            p.drawPath(retPath);
        }
    }

    // Draw arc segments around the wheel perimeter
    for (const auto& arc : arcs) {
        QRectF arcRect(cx - radius, cy - radius, 2 * radius, 2 * radius);

        int startDeg16 = static_cast<int>(arc.startAngle * 180.0 / M_PI * 16);
        int spanDeg16  = static_cast<int>(arc.spanAngle  * 180.0 / M_PI * 16);

        // Filled arc wedge
        p.setPen(Qt::NoPen);
        p.setBrush(arc.color);
        p.drawPie(arcRect, -startDeg16, -spanDeg16);

        // Outline
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(Qt::NoBrush);
        p.drawArc(arcRect, -startDeg16, -spanDeg16);

        // Label outside the arc midpoint
        qreal midAngle = arc.startAngle + arc.spanAngle / 2.0;
        int labelRadius = radius + 20;
        qreal lx = cx + labelRadius * qCos(midAngle);
        qreal ly = cy + labelRadius * qSin(midAngle);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QRectF labelRect(lx - 40, ly - 8, 80, 16);
        p.drawText(labelRect, Qt::AlignCenter, arc.name.left(10));
    }
}

void PaperChordWheel::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    static const QList<QPair<QString, QColor>> catList = {
        {"Citation",      QColor(0x3b, 0x82, 0xf6)},
        {"Collaboration", QColor(0x16, 0xa3, 0x4a)},
        {"Reference",     QColor(0x7c, 0x3a, 0xed)},
        {"Topic",         QColor(0xd9, 0x77, 0x06)}
    };

    auto counts = categoryCounts();

    int itemH = qMin(28, (rect.height() - 50) / qMax(catList.size(), 1));

    for (int i = 0; i < catList.size(); ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        const QColor& color = catList[i].second;
        const QString& name = catList[i].first;

        // Color box
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);

        // Category name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, name);

        // Count
        int cnt = counts.value(name, 0);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(cnt) + " chords");
    }
}

void PaperChordWheel::drawStats(QPainter& p, const QRect& rect) {
    int connCount = 0;
    for (const auto& e : entries_) connCount += e.connections;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Connections", QString::number(connCount),        QColor(0x3b, 0x82, 0xf6)},
        {"Avg Weight",        QString::number(entries_.isEmpty() ? 0.0 : totalWeight() / entries_.size(), 'f', 1), QColor(0x16, 0xa3, 0x4a)},
        {"Bidirectional",     QString::number(bidirectionalCount()), QColor(0x7c, 0x3a, 0xed)},
        {"Entries",           QString::number(entries_.size()),  QColor(0xd9, 0x77, 0x06)}
    };

    int boxH = qMin(42, (rect.height() - 10) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperChordWheel::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QMap<QString, QColor> catColors = {
        {"Citation",      QColor(0x3b, 0x82, 0xf6)},
        {"Collaboration", QColor(0x16, 0xa3, 0x4a)},
        {"Reference",     QColor(0x7c, 0x3a, 0xed)},
        {"Topic",         QColor(0xd9, 0x77, 0x06)}
    };

    QStringList parts = text.split(',', Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        QString segment = part.trimmed();

        int arrowIdx = segment.indexOf("->");
        if (arrowIdx < 0) continue;

        QString source = segment.left(arrowIdx).trimmed();
        QString rest = segment.mid(arrowIdx + 2);

        // Parse optional :weight
        int colonIdx = rest.indexOf(':');
        qreal weight = 50.0;
        QString target = rest;
        if (colonIdx >= 0) {
            target = rest.left(colonIdx).trimmed();
            bool ok = false;
            weight = rest.mid(colonIdx + 1).trimmed().toDouble(&ok);
            if (!ok || weight <= 0) weight = 50.0;
        }

        if (source.isEmpty() || target.isEmpty()) continue;

        int catIdx = categoryCombo_->currentIndex();
        QString category;
        if (catIdx == 0) {
            // "All" - pick based on existing patterns or default
            QStringList cats = {"Citation", "Collaboration", "Reference", "Topic"};
            category = cats[entries_.size() % 4];
        } else {
            category = categoryCombo_->currentText();
        }

        // Detect bidirectional
        bool bidirectional = false;
        for (const auto& existing : entries_) {
            if (existing.source == target && existing.target == source) {
                bidirectional = true;
                break;
            }
        }

        ChordEntry entry;
        entry.id = entries_.size() + 1;
        entry.source = source;
        entry.target = target;
        entry.category = category;
        entry.relation = source + "->" + target;
        entry.weight = weight;
        entry.connections = 1;
        entry.bidirectional = bidirectional;
        entry.color = catColors.value(category, QColor(0x94, 0xa3, 0xb8));

        entries_.append(entry);
        emit chordSelected(entry.id, entry.weight);
    }

    updateInfo();
    saveSettings();
    update();
    inputField_->clear();
}

void PaperChordWheel::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperChordWheel::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Add chord connections to render wheel");
        return;
    }
    int connCount = 0;
    for (const auto& e : entries_) connCount += e.connections;
    infoLabel_->setText(QString("%1 entries | %2 total weight | %3 connections")
        .arg(entries_.size())
        .arg(totalWeight(), 0, 'f', 0)
        .arg(connCount));
}

void PaperChordWheel::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ChordEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.target = settings_.value("target").toString();
        e.category = settings_.value("category").toString();
        e.relation = settings_.value("relation").toString();
        e.weight = settings_.value("weight").toDouble();
        e.connections = settings_.value("connections").toInt();
        e.bidirectional = settings_.value("bidirectional").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperChordWheel::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("relation", entries_[i].relation);
        settings_.setValue("weight", entries_[i].weight);
        settings_.setValue("connections", entries_[i].connections);
        settings_.setValue("bidirectional", entries_[i].bidirectional);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}

QList<ChordEntry> PaperChordWheel::entries() const {
    return entries_;
}

int PaperChordWheel::bidirectionalCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.bidirectional) ++count;
    }
    return count;
}

qreal PaperChordWheel::totalWeight() const {
    qreal total = 0;
    for (const auto& e : entries_) total += e.weight;
    return total;
}

QMap<QString, int> PaperChordWheel::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}
