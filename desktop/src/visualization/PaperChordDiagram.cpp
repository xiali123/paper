#include "visualization/PaperChordDiagram.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <qnumeric.h>
#include <QtMath>

PaperChordDiagram::PaperChordDiagram(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ChordDiagram")
{
    setupUI();
    loadSettings();
}

void PaperChordDiagram::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperChordDiagram::onRender);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citation", "Collaboration", "Topic", "Method"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperChordDiagram::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("source->target:value (e.g. ML->NLP:85)");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Add chord connections to render diagram");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(700, 600);
}

void PaperChordDiagram::addEntry(const ChordEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chordRendered(entry.id, entry.flow);
    update();
}

QList<ChordEntry> PaperChordDiagram::entries() const { return entries_; }

int PaperChordDiagram::bidirectionalCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.bidirectional) ++count;
    }
    return count;
}

qreal PaperChordDiagram::totalFlow() const {
    qreal total = 0;
    for (const auto& e : entries_) total += e.flow;
    return total;
}

QMap<QString, int> PaperChordDiagram::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperChordDiagram::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    // Support multiple comma-separated entries
    QStringList parts = text.split(',', Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        QString segment = part.trimmed();

        // Parse "source->target:value"
        int arrowIdx = segment.indexOf("->");
        if (arrowIdx < 0) continue;

        QString source = segment.left(arrowIdx).trimmed();
        QString rest = segment.mid(arrowIdx + 2);

        int colonIdx = rest.indexOf(':');
        qreal value = 50.0;
        QString target = rest;
        if (colonIdx >= 0) {
            target = rest.left(colonIdx).trimmed();
            bool ok = false;
            value = rest.mid(colonIdx + 1).trimmed().toDouble(&ok);
            if (!ok || value <= 0) value = 50.0;
        }

        if (source.isEmpty() || target.isEmpty()) continue;

        static const QColor palette[] = {
            QColor(0x3b, 0x82, 0xf6), // #3b82f6
            QColor(0x16, 0xa3, 0x4a), // #16a34a
            QColor(0xd9, 0x77, 0x06), // #d97706
            QColor(0xdc, 0x26, 0x26), // #dc2626
            QColor(0x7c, 0x3a, 0xed)  // #7c3aed
        };

        int catIdx = categoryCombo_->currentIndex();
        QString category = (catIdx == 0)
            ? QStringList{"Citation", "Collaboration", "Topic", "Method"}[
                QRandomGenerator::global()->bounded(4)]
            : categoryCombo_->currentText();

        // Compute flow: value normalized with a base weight
        qreal flow = value * (1.0 + qSin(value * 0.1) * 0.2);

        // Compute strength: ratio of flow to a reference max
        qreal strength = qMin(1.0, flow / 100.0);

        // Detect bidirectional: check if reverse connection already exists
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
        entry.value = value;
        entry.strength = strength;
        entry.flow = flow;
        entry.bidirectional = bidirectional;
        entry.color = palette[entries_.size() % 5];

        entries_.append(entry);
    }

    saveSettings();
    updateInfo();
    for (const auto& e : entries_) {
        emit chordRendered(e.id, e.flow);
    }
    update();
    inputField_->clear();
}

void PaperChordDiagram::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Add chord connections to render diagram");
    update();
}

void PaperChordDiagram::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add chord connections to render diagram");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Chord Diagram");

    int w = width(), h = height();
    drawChordView(p, QRect(20, 50, w / 2 + 60, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 90, 50, w / 2 - 110, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 90, h / 2 + 20, w / 2 - 110, h / 2 - 50));
}

void PaperChordDiagram::drawChordView(QPainter& p, const QRect& rect) {
    // Collect all unique nodes from sources and targets
    QSet<QString> nodeSet;
    for (const auto& e : entries_) {
        nodeSet.insert(e.source);
        nodeSet.insert(e.target);
    }
    QList<QString> nodes = nodeSet.values();
    int n = nodes.size();
    if (n < 2) return;

    // Compute per-node total flow for arc sizing
    QMap<QString, qreal> nodeFlow;
    for (const auto& e : entries_) {
        nodeFlow[e.source] += e.flow;
        nodeFlow[e.target] += e.flow;
    }
    qreal totalNodeFlow = 0;
    for (const auto& node : nodes) totalNodeFlow += nodeFlow[node];
    if (totalNodeFlow <= 0) totalNodeFlow = 1.0;

    // Circle geometry
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 40;
    if (radius < 40) radius = 40;

    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    // Map each node to a color
    QMap<QString, QColor> nodeColors;
    for (int i = 0; i < n; ++i) {
        nodeColors[nodes[i]] = palette[i % 5];
    }

    // Assign arc angles proportional to node flow
    // Leave a small gap between arcs
    qreal gapAngle = 0.02; // radians per gap
    qreal totalGap = gapAngle * n;
    qreal availableAngle = 2.0 * M_PI - totalGap;

    struct ArcInfo {
        QString name;
        qreal startAngle;
        qreal spanAngle;
        QColor color;
        qreal flow;
    };
    QList<ArcInfo> arcs;

    qreal currentAngle = -M_PI / 2; // start from top
    for (int i = 0; i < n; ++i) {
        qreal proportion = nodeFlow[nodes[i]] / totalNodeFlow;
        qreal span = proportion * availableAngle;
        ArcInfo arc;
        arc.name = nodes[i];
        arc.startAngle = currentAngle;
        arc.spanAngle = span;
        arc.color = nodeColors[nodes[i]];
        arc.flow = nodeFlow[nodes[i]];
        arcs.append(arc);
        currentAngle += span + gapAngle;
    }

    // Build a quick map from node name to arc info for connection drawing
    QMap<QString, ArcInfo> arcMap;
    for (const auto& arc : arcs) arcMap[arc.name] = arc;

    // Draw the curved connections (chords) first so arcs overlay them
    qreal maxFlow = 1.0;
    for (const auto& e : entries_) maxFlow = qMax(maxFlow, e.flow);

    for (const auto& e : entries_) {
        if (!arcMap.contains(e.source) || !arcMap.contains(e.target)) continue;

        const ArcInfo& srcArc = arcMap[e.source];
        const ArcInfo& tgtArc = arcMap[e.target];

        // Source point on source arc midpoint
        qreal srcMid = srcArc.startAngle + srcArc.spanAngle / 2.0;
        qreal srcX = cx + radius * qCos(srcMid);
        qreal srcY = cy + radius * qSin(srcMid);

        // Target point on target arc midpoint
        qreal tgtMid = tgtArc.startAngle + tgtArc.spanAngle / 2.0;
        qreal tgtX = cx + radius * qCos(tgtMid);
        qreal tgtY = cy + radius * qSin(tgtMid);

        // Control points toward center for a curved ribbon
        qreal pullFactor = 0.3 + 0.4 * (e.flow / maxFlow);
        qreal ctrlSrcX = cx + (srcX - cx) * pullFactor;
        qreal ctrlSrcY = cy + (srcY - cy) * pullFactor;
        qreal ctrlTgtX = cx + (tgtX - cx) * pullFactor;
        qreal ctrlTgtY = cy + (tgtY - cy) * pullFactor;

        int alpha = 40 + static_cast<int>(160 * (e.flow / maxFlow));
        QColor chordColor = e.color;
        chordColor.setAlpha(alpha);

        QPainterPath path;
        path.moveTo(srcX, srcY);
        path.cubicTo(ctrlSrcX, ctrlSrcY, ctrlTgtX, ctrlTgtY, tgtX, tgtY);

        // Draw ribbon thickness proportional to flow
        qreal ribbonWidth = 1.0 + 4.0 * (e.flow / maxFlow);
        QPen pen(chordColor, ribbonWidth);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // If bidirectional, draw a return arc slightly offset
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

            QColor retColor = e.color.lighter(120);
            retColor.setAlpha(alpha);
            QPainterPath retPath;
            retPath.moveTo(t2x, t2y);
            retPath.cubicTo(ct2x, ct2y, cs2x, cs2y, s2x, s2y);
            QPen retPen(retColor, ribbonWidth * 0.7);
            retPen.setCapStyle(Qt::RoundCap);
            p.setPen(retPen);
            p.drawPath(retPath);
        }
    }

    // Draw arc segments around the circle perimeter
    for (const auto& arc : arcs) {
        QRectF arcRect(cx - radius, cy - radius, 2 * radius, 2 * radius);

        // Convert radians to Qt's 1/16th degree system
        int startDeg16 = static_cast<int>(arc.startAngle * 180.0 / M_PI * 16);
        int spanDeg16 = static_cast<int>(arc.spanAngle * 180.0 / M_PI * 16);

        // Fill arc
        p.setPen(Qt::NoPen);
        p.setBrush(arc.color);
        p.drawPie(arcRect, -startDeg16, -spanDeg16);

        // Draw arc outline
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(Qt::NoBrush);
        p.drawArc(arcRect, -startDeg16, -spanDeg16);

        // Label node outside the arc midpoint
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

void PaperChordDiagram::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QList<QString> categories = counts.keys();
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int count = qMin(categories.size(), 5);
    int itemH = qMin(28, (rect.height() - 50) / qMax(count, 1));

    for (int i = 0; i < count; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        QColor color = palette[i % 5];

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i].left(12));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        int cnt = counts[categories[i]];
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(cnt) + " chords");
    }
}

void PaperChordDiagram::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Chords", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Bidirectional", QString::number(bidirectionalCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Total Flow", QString::number(totalFlow(), 'f', 0), QColor(0xd9, 0x77, 0x06)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c, 0x3a, 0xed)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
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

void PaperChordDiagram::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Add chord connections to render diagram");
        return;
    }
    infoLabel_->setText(QString("%1 chords | %2 bidirectional | %3 total flow")
        .arg(entries_.size())
        .arg(bidirectionalCount())
        .arg(totalFlow(), 0, 'f', 0));
}

void PaperChordDiagram::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ChordEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.target = settings_.value("target").toString();
        e.category = settings_.value("category").toString();
        e.value = settings_.value("value").toDouble();
        e.strength = settings_.value("strength").toDouble();
        e.flow = settings_.value("flow").toDouble();
        e.bidirectional = settings_.value("bidirectional").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperChordDiagram::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("flow", entries_[i].flow);
        settings_.setValue("bidirectional", entries_[i].bidirectional);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
