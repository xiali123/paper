#include "visualization/PaperDonutRing.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <QPainterPath>

PaperDonutRing::PaperDonutRing(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DonutRing")
{
    setupUI();
    loadSettings();
}

void PaperDonutRing::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Toolbar row
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Inner", "Middle", "Outer", "All Rings"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Segment:value");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperDonutRing::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDonutRing::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render donut ring chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 550);
}

void PaperDonutRing::addEntry(const DonutRingEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit segmentClicked(entry.id, entry.value);
    update();
}

QList<DonutRingEntry> PaperDonutRing::entries() const { return entries_; }

int PaperDonutRing::outerCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.outer) ++c;
    return c;
}

qreal PaperDonutRing::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

QMap<QString, int> PaperDonutRing::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDonutRing::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#7c3aed"), QColor("#d97706")
    };

    // Parse "Segment:value" format
    QString segmentName = text;
    qreal val = 10 + QRandomGenerator::global()->bounded(90);
    int colonPos = text.indexOf(':');
    if (colonPos > 0) {
        segmentName = text.left(colonPos);
        bool ok = false;
        double parsed = text.mid(colonPos + 1).toDouble(&ok);
        if (ok && parsed > 0) val = parsed;
    }

    // Determine ring from combo
    QString filter = categoryCombo_->currentText();
    QString ring;
    if (filter == "Inner") ring = "inner";
    else if (filter == "Middle") ring = "middle";
    else if (filter == "Outer") ring = "outer";
    else {
        QStringList rings = {"inner", "middle", "outer"};
        ring = rings[QRandomGenerator::global()->bounded(rings.size())];
    }

    // Pick a category color
    static const QStringList categories = {"Method", "Data", "Result", "Theory"};
    QString cat = categories[entries_.size() % categories.size()];
    QColor color = palette[entries_.size() % 4];

    DonutRingEntry entry;
    entry.id = entries_.size() + 1;
    entry.segment = segmentName;
    entry.category = cat;
    entry.ring = ring;
    entry.value = val;
    entry.outer = (ring == "outer");
    entry.color = color;

    entries_.append(entry);

    // Recompute percentages relative to total
    qreal total = 0;
    for (const auto& e : entries_) total += e.value;
    for (auto& e : entries_) {
        e.percentage = qRound((e.value / qMax(total, 1.0)) * 100.0);
    }

    saveSettings();
    updateInfo();
    emit segmentClicked(entry.id, entry.value);
    update();
    inputField_->clear();
}

void PaperDonutRing::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperDonutRing::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render donut ring chart");
        return;
    }

    int w = width(), h = height();

    // Top: donut rings visualization
    int topMargin = 50;
    int drawH = h - topMargin - 20;
    int drawW = w - 40;
    drawDonutRings(p, QRect(10, topMargin, drawW / 2, drawH));

    // Bottom-left: category legend
    drawCategoryLegend(p, QRect(drawW / 2 + 20, topMargin, drawW / 2, drawH / 2));

    // Bottom-right: stats
    drawStats(p, QRect(drawW / 2 + 20, topMargin + drawH / 2 + 10, drawW / 2, drawH / 2 - 10));
}

void PaperDonutRing::drawDonutRings(QPainter& p, const QRect& area) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(area.x() + 10, area.y() + 18, "Donut Rings");

    // Group entries by ring
    QMap<QString, QList<DonutRingEntry>> ringGroups;
    for (const auto& e : entries_) {
        ringGroups[e.ring].append(e);
    }

    // Determine active rings and their order (inner -> middle -> outer)
    QStringList ringOrder;
    if (ringGroups.contains("inner")) ringOrder << "inner";
    if (ringGroups.contains("middle")) ringOrder << "middle";
    if (ringGroups.contains("outer")) ringOrder << "outer";
    // Any unexpected ring names
    for (const auto& key : ringGroups.keys()) {
        if (!ringOrder.contains(key)) ringOrder << key;
    }

    int numRings = ringOrder.size();
    if (numRings == 0) return;

    // Geometry: concentric rings
    int cx = area.x() + area.width() / 2;
    int cy = area.y() + 40 + (area.height() - 60) / 2;
    int maxRadius = qMin(area.width(), area.height() - 60) / 2 - 20;
    int holeRadius = maxRadius * 0.25; // center hole
    int ringWidth = (maxRadius - holeRadius) / numRings;

    // Draw rings from outermost to innermost so inner rings paint over outer
    for (int ri = numRings - 1; ri >= 0; --ri) {
        const QString& ringName = ringOrder[ri];
        const QList<DonutRingEntry>& ringEntries = ringGroups[ringName];

        qreal ringTotal = 0;
        for (const auto& e : ringEntries) ringTotal += e.value;
        if (ringTotal <= 0) continue;

        // Outer edge of this ring; inner rings are closer to center
        int outerR = holeRadius + ringWidth * (ri + 1);
        int innerR = holeRadius + ringWidth * ri;

        qreal startAngle = 0.0; // in degrees, Qt 0 is at 3 o'clock
        for (const auto& e : ringEntries) {
            qreal span = (e.value / ringTotal) * 360.0;

            // Build arc path using QPainterPath for precise donut segments
            QPainterPath segmentPath;
            QRectF outerRect(cx - outerR, cy - outerR, outerR * 2, outerR * 2);
            QRectF innerRect(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

            qreal startRad = qDegreesToRadians(-startAngle + 90); // flip for Qt coords
            qreal endRad = qDegreesToRadians(-(startAngle + span) + 90);
            qreal midAngleDeg = startAngle + span / 2.0;

            // Outer arc
            segmentPath.arcMoveTo(outerRect, startAngle);
            segmentPath.arcTo(outerRect, startAngle, span);
            // Line to inner arc end
            QPointF innerEnd = segmentPath.currentPosition();
            // Inner arc (reverse direction)
            segmentPath.arcTo(innerRect, startAngle + span, -span);
            segmentPath.closeSubpath();

            p.setPen(QPen(Qt::white, 1.5));
            p.setBrush(e.color);
            p.drawPath(segmentPath);

            // Percentage label for segments wider than ~15 degrees
            if (span > 15.0) {
                qreal midRad = qDegreesToRadians(midAngleDeg);
                int labelR = (outerR + innerR) / 2;
                int tx = cx + static_cast<int>(labelR * qCos(midRad));
                int ty = cy - static_cast<int>(labelR * qSin(midRad));
                p.setPen(Qt::white);
                p.setFont(QFont("Arial", 8, QFont::Bold));
                p.drawText(tx - 18, ty - 6, 36, 14, Qt::AlignCenter,
                           QString::number(e.percentage) + "%");
            }

            startAngle += span;
        }

        // Ring label
        int labelR = (outerR + innerR) / 2;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(cx - 20, cy - outerR - 4, 40, 12, Qt::AlignCenter, ringName);
    }

    // Center hole
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(cx - holeRadius, cy - holeRadius, holeRadius * 2, holeRadius * 2);

    // Center total
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    QString totalText = QString::number(static_cast<int>(totalValue()));
    QFontMetrics fm(p.font());
    int tw = fm.horizontalAdvance(totalText);
    p.drawText(cx - tw / 2, cy + fm.ascent() / 2 - 2, totalText);
}

void PaperDonutRing::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Category Legend");

    QMap<QString, int> counts = categoryCounts();
    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#7c3aed"), QColor("#d97706")
    };
    static const QStringList knownCategories = {"Method", "Data", "Result", "Theory"};

    int startY = rect.y() + 30;
    int itemH = 26;
    int idx = 0;

    // Show known categories first, then any extras
    QStringList allCats;
    for (const auto& c : knownCategories) {
        if (counts.contains(c)) allCats << c;
    }
    for (const auto& c : counts.keys()) {
        if (!allCats.contains(c)) allCats << c;
    }

    int maxItems = (rect.height() - 30) / itemH;
    int show = qMin(allCats.size(), maxItems);

    for (int i = 0; i < show; ++i) {
        const QString& cat = allCats[i];
        int y = startY + i * itemH;

        int colorIdx = knownCategories.indexOf(cat);
        QColor color = (colorIdx >= 0) ? palette[colorIdx] : QColor("#64748b");

        // Color box
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);

        // Category name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 25, y + 2, rect.width() / 2 - 30, 18,
                   Qt::AlignVCenter, cat);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 10,
                   18, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(counts[cat]) + " entries");
        ++idx;
    }
}

void PaperDonutRing::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries",   QString::number(entries_.size()),   QColor("#3b82f6")},
        {"Total Value",     QString::number(totalValue(), 'f', 0), QColor("#16a34a")},
        {"Outer Count",     QString::number(outerCount()),      QColor("#7c3aed")},
        {"Categories",      QString::number(categoryCounts().size()), QColor("#d97706")}
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

void PaperDonutRing::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render donut ring chart");
        return;
    }
    infoLabel_->setText(
        QString("Entries: %1 | Outer: %2 | Total: %3 | Categories: %4")
            .arg(entries_.size())
            .arg(outerCount())
            .arg(totalValue(), 0, 'f', 0)
            .arg(categoryCounts().size()));
}

void PaperDonutRing::loadSettings() {
    settings_.beginGroup("DonutRing");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DonutRingEntry e;
        e.id = settings_.value("id").toInt();
        e.segment = settings_.value("segment").toString();
        e.category = settings_.value("category").toString();
        e.ring = settings_.value("ring").toString();
        e.value = settings_.value("value").toDouble();
        e.percentage = settings_.value("percentage").toInt();
        e.outer = settings_.value("outer").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperDonutRing::saveSettings() {
    settings_.beginGroup("DonutRing");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("segment", entries_[i].segment);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("ring", entries_[i].ring);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("percentage", entries_[i].percentage);
        settings_.setValue("outer", entries_[i].outer);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
