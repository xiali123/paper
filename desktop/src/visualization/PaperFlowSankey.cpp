#include "visualization/PaperFlowSankey.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSet>
#include <QRandomGenerator>

PaperFlowSankey::PaperFlowSankey(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FlowSankey")
{
    setupUI();
    loadSettings();
}

void PaperFlowSankey::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Energy", "Material", "Information", "Finance"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Source->Target:flow");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperFlowSankey::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFlowSankey::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add flow entries and render");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 520);
}

void PaperFlowSankey::addEntry(const FlowSankeyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit flowSelected(entry.id, entry.flow);
    update();
}

QList<FlowSankeyEntry> PaperFlowSankey::entries() const { return entries_; }

int PaperFlowSankey::bidirectionalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.bidirectional) ++c;
    return c;
}

qreal PaperFlowSankey::totalFlow() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.flow;
    return t;
}

QMap<QString, int> PaperFlowSankey::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFlowSankey::onRender() {
    QString text = inputField_->text().trimmed();

    if (!text.isEmpty()) {
        QStringList parts = text.split(":");
        if (parts.size() >= 2) {
            qreal flow = parts.last().toDouble();
            QString srcTgt = parts.first();
            QStringList stParts = srcTgt.split("->");
            if (stParts.size() == 2 && flow > 0) {
                FlowSankeyEntry e;
                e.id = entries_.size() + 1;
                e.source = stParts[0].trimmed();
                e.target = stParts[1].trimmed();

                QString cat = categoryCombo_->currentText();
                if (cat == "All") cat = "Energy";
                e.category = cat;
                e.flow = flow;
                e.paths = 1;
                e.bidirectional = false;

                static QMap<QString, QColor> catColor = {
                    {"Energy",       QColor(0x3b, 0x82, 0xf6)},
                    {"Material",     QColor(0x16, 0xa3, 0x4a)},
                    {"Information",  QColor(0x7c, 0x3a, 0xed)},
                    {"Finance",      QColor(0xd9, 0x77, 0x06)}
                };
                e.color = catColor.value(e.category, QColor(0x3b, 0x82, 0xf6));
                entries_.append(e);
            }
        }
    } else {
        entries_.clear();

        QStringList sources = {"Solar", "Wind", "Hydro", "Geothermal", "Biomass"};
        QStringList targets = {"Industry", "Transport", "Residential", "Commercial", "Agriculture"};
        QStringList categories = {"Energy", "Material", "Information", "Finance"};

        static QMap<QString, QColor> catColor = {
            {"Energy",       QColor(0x3b, 0x82, 0xf6)},
            {"Material",     QColor(0x16, 0xa3, 0x4a)},
            {"Information",  QColor(0x7c, 0x3a, 0xed)},
            {"Finance",      QColor(0xd9, 0x77, 0x06)}
        };

        QString filter = categoryCombo_->currentText();
        int count = 8 + QRandomGenerator::global()->bounded(7);
        for (int i = 0; i < count; ++i) {
            FlowSankeyEntry e;
            e.id = entries_.size() + 1;
            e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
            e.target = targets[QRandomGenerator::global()->bounded(targets.size())];

            if (filter == "All")
                e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            else
                e.category = filter;

            e.flow = 5.0 + QRandomGenerator::global()->bounded(96);
            e.paths = 1 + QRandomGenerator::global()->bounded(4);
            e.bidirectional = QRandomGenerator::global()->bounded(5) == 0;
            e.color = catColor.value(e.category, QColor(0x3b, 0x82, 0xf6));
            entries_.append(e);
        }
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit flowSelected(entries_.last().id, entries_.last().flow);
    inputField_->clear();
    update();
}

void PaperFlowSankey::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Add flow entries and render");
    update();
}

void PaperFlowSankey::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add flow entries and render");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Flow Sankey Diagram");

    int w = width(), h = height();
    drawSankey(p, QRect(20, 50, w - 40, h - 120));
    drawCategoryLegend(p, QRect(20, h - 60, w / 2 - 30, 50));
    drawStats(p, QRect(w / 2 + 10, h - 60, w / 2 - 30, 50));
}

void PaperFlowSankey::drawSankey(QPainter& p, const QRect& rect) {
    // Determine filtered entries
    QString filter = categoryCombo_->currentText();
    QList<int> filtered;
    for (int i = 0; i < entries_.size(); ++i) {
        if (filter == "All" || entries_[i].category == filter)
            filtered.append(i);
    }
    if (filtered.isEmpty()) return;

    // Collect unique sources and targets in filtered set, preserving order
    QStringList sourceList, targetList;
    QSet<QString> seenSrc, seenTgt;
    for (int idx : filtered) {
        const auto& e = entries_[idx];
        if (!seenSrc.contains(e.source)) {
            seenSrc.insert(e.source);
            sourceList.append(e.source);
        }
        if (!seenTgt.contains(e.target)) {
            seenTgt.insert(e.target);
            targetList.append(e.target);
        }
    }

    int numSrc = sourceList.size();
    int numTgt = targetList.size();
    if (numSrc == 0 || numTgt == 0) return;

    // Compute max flow for proportional sizing
    qreal maxFlow = 1.0;
    for (int idx : filtered)
        maxFlow = qMax(maxFlow, entries_[idx].flow);

    // Layout parameters
    int leftX = rect.x() + 80;
    int rightX = rect.x() + rect.width() - 80;
    int topY = rect.y() + 10;
    int bottomY = rect.y() + rect.height() - 10;
    int availableH = bottomY - topY;

    // Compute source node positions and heights (proportional to outgoing flow)
    QMap<QString, qreal> srcTotalFlow;
    for (int idx : filtered) {
        const auto& e = entries_[idx];
        srcTotalFlow[e.source] += e.flow;
    }
    qreal grandSrcFlow = 0;
    for (const auto& v : srcTotalFlow) grandSrcFlow += v;

    struct NodePos { qreal y; qreal h; };
    QMap<QString, NodePos> srcPos;
    {
        qreal curY = topY;
        for (int i = 0; i < numSrc; ++i) {
            qreal frac = grandSrcFlow > 0 ? srcTotalFlow[sourceList[i]] / grandSrcFlow : 1.0 / numSrc;
            qreal nodeH = qMax(8.0, frac * availableH - 3.0);
            srcPos[sourceList[i]] = {curY + nodeH / 2.0, nodeH};
            curY += nodeH + 3.0;
        }
    }

    // Compute target node positions and heights (proportional to incoming flow)
    QMap<QString, qreal> tgtTotalFlow;
    for (int idx : filtered) {
        const auto& e = entries_[idx];
        tgtTotalFlow[e.target] += e.flow;
    }
    qreal grandTgtFlow = 0;
    for (const auto& v : tgtTotalFlow) grandTgtFlow += v;

    QMap<QString, NodePos> tgtPos;
    {
        qreal curY = topY;
        for (int i = 0; i < numTgt; ++i) {
            qreal frac = grandTgtFlow > 0 ? tgtTotalFlow[targetList[i]] / grandTgtFlow : 1.0 / numTgt;
            qreal nodeH = qMax(8.0, frac * availableH - 3.0);
            tgtPos[targetList[i]] = {curY + nodeH / 2.0, nodeH};
            curY += nodeH + 3.0;
        }
    }

    // Draw source nodes (vertical bars on the left)
    int nodeBarW = 14;
    for (int i = 0; i < numSrc; ++i) {
        const auto& np = srcPos[sourceList[i]];
        QColor srcColor(0x3b, 0x82, 0xf6);
        p.setPen(Qt::NoPen);
        p.setBrush(srcColor);
        p.drawRoundedRect(QRectF(leftX - nodeBarW, np.y - np.h / 2.0, nodeBarW, np.h), 3.0, 3.0);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRectF(leftX - nodeBarW - 76, np.y - np.h / 2.0, 72, np.h),
                   Qt::AlignRight | Qt::AlignVCenter, sourceList[i]);
    }

    // Draw target nodes (vertical bars on the right)
    for (int i = 0; i < numTgt; ++i) {
        const auto& np = tgtPos[targetList[i]];
        QColor tgtColor(0x16, 0xa3, 0x4a);
        p.setPen(Qt::NoPen);
        p.setBrush(tgtColor);
        p.drawRoundedRect(QRectF(rightX, np.y - np.h / 2.0, nodeBarW, np.h), 3.0, 3.0);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRectF(rightX + nodeBarW + 4, np.y - np.h / 2.0, 72, np.h),
                   Qt::AlignLeft | Qt::AlignVCenter, targetList[i]);
    }

    // Draw flow paths between source and target nodes
    // For each source, track the current Y offset for flow band allocation
    QMap<QString, qreal> srcFlowYOffset;
    for (const auto& s : sourceList)
        srcFlowYOffset[s] = srcPos[s].y - srcPos[s].h / 2.0;

    QMap<QString, qreal> tgtFlowYOffset;
    for (const auto& t : targetList)
        tgtFlowYOffset[t] = tgtPos[t].y - tgtPos[t].h / 2.0;

    for (int idx : filtered) {
        const auto& e = entries_[idx];
        if (!srcPos.contains(e.source) || !tgtPos.contains(e.target)) continue;

        qreal srcNodeH = srcPos[e.source].h;
        qreal srcTotal = srcTotalFlow[e.source];
        qreal bandFraction = srcTotal > 0 ? e.flow / srcTotal : 0;
        qreal bandH = qMax(2.0, bandFraction * srcNodeH);

        qreal srcYStart = srcFlowYOffset[e.source];
        qreal srcYCenter = srcYStart + bandH / 2.0;
        srcFlowYOffset[e.source] += bandH;

        qreal tgtNodeH = tgtPos[e.target].h;
        qreal tgtTotal = tgtTotalFlow[e.target];
        qreal tgtBandFrac = tgtTotal > 0 ? e.flow / tgtTotal : 0;
        qreal tgtBandH = qMax(2.0, tgtBandFrac * tgtNodeH);

        qreal tgtYStart = tgtFlowYOffset[e.target];
        qreal tgtYCenter = tgtYStart + tgtBandH / 2.0;
        tgtFlowYOffset[e.target] += tgtBandH;

        // Build the curved sankey path
        qreal x1 = leftX;
        qreal x2 = rightX;
        qreal midX = (x1 + x2) / 2.0;

        QPainterPath path;
        path.moveTo(x1, srcYCenter);
        path.cubicTo(midX, srcYCenter, midX, tgtYCenter, x2, tgtYCenter);

        // Close the path for a filled band
        path.lineTo(x2, tgtYCenter + tgtBandH);
        path.cubicTo(midX, tgtYCenter + tgtBandH, midX, srcYCenter + bandH, x1, srcYCenter + bandH);
        path.closeSubpath();

        int alpha = 60 + static_cast<int>((e.flow / maxFlow) * 120);
        QColor fill = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);

        if (e.bidirectional) {
            QPen dashPen(QColor(e.color.red(), e.color.green(), e.color.blue(), 180), 1.0, Qt::DashLine);
            p.setPen(dashPen);
        } else {
            p.setPen(Qt::NoPen);
        }
        p.setBrush(fill);
        p.drawPath(path);

        // Draw flow value label on the path midpoint
        qreal midY = (srcYCenter + tgtYCenter) / 2.0 + bandH / 2.0;
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(QRectF(midX - 20, midY - 10, 40, 14), Qt::AlignCenter,
                   QString::number(static_cast<int>(e.flow)));
    }
}

void PaperFlowSankey::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 12, "Categories");

    auto counts = categoryCounts();

    struct CatInfo { QString name; QColor color; };
    QList<CatInfo> cats = {
        {"Energy",       QColor(0x3b, 0x82, 0xf6)},
        {"Material",     QColor(0x16, 0xa3, 0x4a)},
        {"Information",  QColor(0x7c, 0x3a, 0xed)},
        {"Finance",      QColor(0xd9, 0x77, 0x06)}
    };

    int boxW = (rect.width() - 10) / cats.size();
    for (int i = 0; i < cats.size(); ++i) {
        int x = rect.x() + i * (boxW + 2);
        int y = rect.y() + 18;
        int count = counts.contains(cats[i].name) ? counts[cats[i].name] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(cats[i].color);
        p.drawRoundedRect(x, y, 12, 12, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x + 16, y, boxW - 18, 14, Qt::AlignVCenter,
                   cats[i].name);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 16, y + 14, boxW - 18, 12, Qt::AlignVCenter,
                   QString::number(count) + " flows");
    }
}

void PaperFlowSankey::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Flow",         QString::number(totalFlow(), 'f', 0),    QColor(0x3b, 0x82, 0xf6)},
        {"Bidirectional",      QString::number(bidirectionalCount()),   QColor(0xd9, 0x77, 0x06)},
        {"Paths",              QString::number(entries_.size()),        QColor(0x16, 0xa3, 0x4a)},
    };

    int boxW = qMin(100, (rect.width() - 10) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 5);
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, 44, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 13, QFont::Bold));
        p.drawText(x + 6, y + 3, boxW - 12, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 6, y + 26, boxW - 12, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperFlowSankey::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Add flow entries and render");
        return;
    }
    infoLabel_->setText(QString("%1 entries | flow: %2 | bidir: %3")
        .arg(entries_.size())
        .arg(totalFlow(), 0, 'f', 0)
        .arg(bidirectionalCount()));
}

void PaperFlowSankey::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FlowSankeyEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.category = settings_.value("category").toString();
        e.target = settings_.value("target").toString();
        e.flow = settings_.value("flow").toDouble();
        e.paths = settings_.value("paths").toInt();
        e.bidirectional = settings_.value("bidirectional").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFlowSankey::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("flow", entries_[i].flow);
        settings_.setValue("paths", entries_[i].paths);
        settings_.setValue("bidirectional", entries_[i].bidirectional);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
