#include "tools/PaperNoveltyRadar2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

// ── construction ──────────────────────────────────────────────────────────

PaperNoveltyRadar2::PaperNoveltyRadar2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NoveltyRadar2")
{
    setupUI();
    loadSettings();
}

void PaperNoveltyRadar2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Toolbar row
    auto* toolbar = new QHBoxLayout();

    dimensionCombo_ = new QComboBox();
    dimensionCombo_->addItems({"All Dimensions", "Methodology", "Dataset", "Architecture", "Application", "Theory"});
    dimensionCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 120px; }");
    toolbar->addWidget(dimensionCombo_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All Categories", "NLP", "Vision", "Multimodal", "Audio", "Robotics"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 100px; }");
    toolbar->addWidget(categoryCombo_);

    addBtn_ = new QPushButton("Seed Data");
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperNoveltyRadar2::onSeed);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #dc2626; "
        "border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNoveltyRadar2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Novelty Radar");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(780, 580);
}

// ── data helpers ──────────────────────────────────────────────────────────

void PaperNoveltyRadar2::addEntry(const NoveltyRadar2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<NoveltyRadar2Entry> PaperNoveltyRadar2::entries() const {
    return entries_;
}

int PaperNoveltyRadar2::breakthroughCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.breakthrough) ++c;
    return c;
}

qreal PaperNoveltyRadar2::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

int PaperNoveltyRadar2::totalCitations() const {
    int c = 0;
    for (const auto& e : entries_) c += e.citations;
    return c;
}

QMap<QString, int> PaperNoveltyRadar2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ── slots ─────────────────────────────────────────────────────────────────

void PaperNoveltyRadar2::onSeed() {
    static const QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    struct SeedData {
        QString paper;
        QString category;
        QString dimension;
        qreal score;
        int citations;
        bool breakthrough;
    };

    SeedData seeds[] = {
        {"Attention Is All You Need", "NLP",        "Architecture",   9.5, 120000, true },
        {"Attention Is All You Need", "NLP",        "Methodology",    9.8, 120000, true },
        {"BERT",                      "NLP",        "Methodology",    9.2,  95000, true },
        {"BERT",                      "NLP",        "Application",    8.8,  95000, true },
        {"GPT-4",                     "Multimodal", "Architecture",   9.6,  28000, true },
        {"GPT-4",                     "Multimodal", "Application",    9.4,  28000, true },
        {"DALL-E",                    "Vision",     "Architecture",   8.5,  14000, false},
        {"DALL-E",                    "Vision",     "Dataset",        7.8,  14000, false},
    };

    for (int i = 0; i < 8; ++i) {
        NoveltyRadar2Entry e;
        e.id          = entries_.size() + 1;
        e.paper       = seeds[i].paper;
        e.category    = seeds[i].category;
        e.dimension   = seeds[i].dimension;
        e.score       = seeds[i].score;
        e.citations   = seeds[i].citations;
        e.breakthrough = seeds[i].breakthrough;
        e.color       = palette[i % 5];
        addEntry(e);
        emit noveltyAdded(e.id, e.score);
    }
}

void PaperNoveltyRadar2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperNoveltyRadar2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Novelty Radar");
        return;
    }
    infoLabel_->setText(QString("Entries: %1 | Breakthroughs: %2 | Avg Score: %3 | Citations: %4")
        .arg(entries_.size())
        .arg(breakthroughCount())
        .arg(avgScore(), 0, 'f', 1)
        .arg(totalCitations()));
}

// ── persistence ───────────────────────────────────────────────────────────

void PaperNoveltyRadar2::loadSettings() {
    settings_.beginGroup("NoveltyRadar2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NoveltyRadar2Entry e;
        e.id           = settings_.value("id").toInt();
        e.paper        = settings_.value("paper").toString();
        e.category     = settings_.value("category").toString();
        e.dimension    = settings_.value("dimension").toString();
        e.score        = settings_.value("score").toDouble();
        e.citations    = settings_.value("citations").toInt();
        e.breakthrough = settings_.value("breakthrough").toBool();
        e.color        = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperNoveltyRadar2::saveSettings() {
    settings_.beginGroup("NoveltyRadar2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",           entries_[i].id);
        settings_.setValue("paper",        entries_[i].paper);
        settings_.setValue("category",     entries_[i].category);
        settings_.setValue("dimension",    entries_[i].dimension);
        settings_.setValue("score",        entries_[i].score);
        settings_.setValue("citations",    entries_[i].citations);
        settings_.setValue("breakthrough", entries_[i].breakthrough);
        settings_.setValue("color",        entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

// ── painting ──────────────────────────────────────────────────────────────

void PaperNoveltyRadar2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 0, width() - 40, 28, Qt::AlignLeft | Qt::AlignVCenter,
               "Novelty Radar");

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No data yet - click Seed Data");
        return;
    }

    int w = width();
    int h = height();
    int toolbarH = 70;

    // Layout: radar chart top half, bar chart bottom-left, stats bottom-right
    int topH    = (h - toolbarH) * 55 / 100;
    int bottomH = (h - toolbarH) - topH - 10;

    drawRadarView(p, QRect(10, toolbarH, w - 20, topH));
    drawCategoryChart(p, QRect(10, toolbarH + topH + 5, (w - 30) / 2, bottomH));
    drawStats(p, QRect(20 + (w - 30) / 2, toolbarH + topH + 5, (w - 30) / 2, bottomH));
}

// ── radar / spider chart ──────────────────────────────────────────────────

void PaperNoveltyRadar2::drawRadarView(QPainter& p, const QRect& area) {
    // Card background
    QPainterPath card;
    card.addRoundedRect(area, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawPath(card);
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(card);

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(area.x() + 12, area.y() + 4, area.width() - 24, 22,
               Qt::AlignLeft | Qt::AlignVCenter, "Novelty Radar Chart");

    // 5 dimensions as axes
    static const QString dimensions[] = {
        "Methodology", "Dataset", "Architecture", "Application", "Theory"
    };
    const int dimCount = 5;

    // Collect unique papers for polygon drawing
    QMap<QString, QList<NoveltyRadar2Entry>> paperEntries;
    QString dimFilter = dimensionCombo_->currentText();
    QString catFilter = categoryCombo_->currentText();
    for (const auto& e : entries_) {
        if (dimFilter != "All Dimensions" && e.dimension != dimFilter) continue;
        if (catFilter != "All Categories" && e.category != catFilter) continue;
        paperEntries[e.paper].append(e);
    }

    if (paperEntries.isEmpty()) {
        p.setPen(QColor(160, 174, 192));
        p.setFont(QFont("Arial", 9));
        p.drawText(area, Qt::AlignCenter, "No entries for current filter");
        return;
    }

    // Radar geometry
    int cx = area.x() + area.width() / 2;
    int cy = area.y() + area.height() / 2 + 8;
    int radius = qMin(area.width(), area.height()) / 2 - 60;
    if (radius < 40) radius = 40;

    auto angleForAxis = [&](int idx) -> qreal {
        // Start from top (270 degrees), go clockwise
        return qDegreesToRadians(270.0 + idx * 360.0 / dimCount);
    };

    // Draw concentric pentagon rings (20%, 40%, 60%, 80%, 100%)
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    for (int ring = 1; ring <= 5; ++ring) {
        qreal r = radius * ring / 5.0;
        QPolygonF ringPoly;
        for (int d = 0; d < dimCount; ++d) {
            qreal angle = angleForAxis(d);
            ringPoly << QPointF(cx + r * qCos(angle), cy + r * qSin(angle));
        }
        ringPoly << ringPoly.first();
        p.drawPolyline(ringPoly);
    }

    // Draw axis lines and labels
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.setFont(QFont("Arial", 8));
    for (int d = 0; d < dimCount; ++d) {
        qreal angle = angleForAxis(d);
        qreal ex = cx + radius * qCos(angle);
        qreal ey = cy + radius * qSin(angle);
        p.drawLine(QPointF(cx, cy), QPointF(ex, ey));

        // Label offset
        qreal lx = cx + (radius + 18) * qCos(angle);
        qreal ly = cy + (radius + 18) * qSin(angle);
        p.setPen(QColor(71, 85, 105));
        p.drawText(QRectF(lx - 40, ly - 8, 80, 16), Qt::AlignCenter, dimensions[d]);
        p.setPen(QPen(QColor(203, 213, 225), 1));
    }

    // Draw each paper as a polygon
    static const QColor paperColors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int colorIdx = 0;
    QMap<QString, bool> paperBreakthrough;
    for (const auto& e : entries_) {
        if (e.breakthrough) paperBreakthrough[e.paper] = true;
    }

    for (auto it = paperEntries.constBegin(); it != paperEntries.constEnd(); ++it) {
        QColor col = paperColors[colorIdx % 5];
        colorIdx++;

        // Build dimension-to-score map for this paper
        QMap<QString, qreal> dimScores;
        for (const auto& e : it.value()) {
            // If multiple entries for same dimension, take max
            if (!dimScores.contains(e.dimension) || e.score > dimScores[e.dimension])
                dimScores[e.dimension] = e.score;
        }

        // Build polygon points
        QPolygonF poly;
        QList<QPointF> starPositions;
        for (int d = 0; d < dimCount; ++d) {
            qreal score = dimScores.contains(dimensions[d]) ? dimScores[dimensions[d]] : 0.0;
            qreal r = (score / 10.0) * radius;
            qreal angle = angleForAxis(d);
            QPointF pt(cx + r * qCos(angle), cy + r * qSin(angle));
            poly << pt;

            // Collect star positions for breakthrough papers
            if (paperBreakthrough.contains(it.key()) && score > 0) {
                starPositions << pt;
            }
        }
        poly << poly.first();

        // Filled polygon with transparency
        p.setPen(QPen(col, 2));
        QColor fillCol = col;
        fillCol.setAlpha(40);
        p.setBrush(fillCol);
        p.drawPolygon(poly);

        // Star markers for breakthrough papers
        bool isBreakthrough = paperBreakthrough.contains(it.key());
        for (const auto& pt : starPositions) {
            if (isBreakthrough) {
                drawStar(p, pt, 7, col);
            }
        }

        // Paper name label near the first data point
        if (!poly.isEmpty() && poly.size() > 1) {
            QPointF labelPt = poly.first();
            p.setPen(col);
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(QRectF(labelPt.x() - 50, labelPt.y() - 22, 100, 14),
                       Qt::AlignCenter, it.key());
        }
    }

    // Legend at bottom of radar card
    int legendY = area.y() + area.height() - 22;
    int legendX = area.x() + 16;
    colorIdx = 0;
    p.setFont(QFont("Arial", 7));
    for (auto it = paperEntries.constBegin(); it != paperEntries.constEnd(); ++it) {
        QColor col = paperColors[colorIdx % 5];
        colorIdx++;

        // Color swatch
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);

        // Paper name
        p.setPen(QColor(71, 85, 105));
        p.drawText(legendX + 14, legendY + 10, it.key());

        // Breakthrough star indicator
        if (paperBreakthrough.contains(it.key())) {
            drawStar(p, QPointF(legendX + 14 + p.fontMetrics().horizontalAdvance(it.key()) + 8,
                                legendY + 5), 5, col);
        }

        legendX += p.fontMetrics().horizontalAdvance(it.key()) + 36;
    }
}

void PaperNoveltyRadar2::drawStar(QPainter& p, const QPointF& center, qreal size, const QColor& color) {
    p.save();
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    QPolygonF star;
    for (int i = 0; i < 10; ++i) {
        qreal angle = qDegreesToRadians(-90.0 + i * 36.0);
        qreal r = (i % 2 == 0) ? size : size * 0.45;
        star << QPointF(center.x() + r * qCos(angle), center.y() + r * qSin(angle));
    }
    p.drawPolygon(star);
    p.restore();
}

// ── horizontal bar chart ──────────────────────────────────────────────────

void PaperNoveltyRadar2::drawCategoryChart(QPainter& p, const QRect& area) {
    // Card background
    QPainterPath card;
    card.addRoundedRect(area, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawPath(card);
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(card);

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(area.x() + 12, area.y() + 4, area.width() - 24, 22,
               Qt::AlignLeft | Qt::AlignVCenter, "Category Breakdown");

    struct CatInfo { QString label; QColor color; };
    CatInfo cats[] = {
        {"NLP",        QColor(59, 130, 246)},   // #3b82f6
        {"Vision",     QColor(22, 163, 74)},    // #16a34a
        {"Multimodal", QColor(217, 119, 6)},    // #d97706
        {"Audio",      QColor(220, 38, 38)},    // #dc2626
        {"Robotics",   QColor(124, 58, 237)}    // #7c3aed
    };

    auto counts = categoryCounts();
    int maxVal = 1;
    for (const auto& c : cats)
        maxVal = qMax(maxVal, counts.contains(c.label) ? counts[c.label] : 0);

    int chartTop = area.y() + 32;
    int barH     = qMin(20, (area.height() - 50) / 5);
    int maxBarW  = area.width() - 140;
    int labelW   = 66;

    for (int i = 0; i < 5; ++i) {
        int y     = chartTop + i * (barH + 8);
        int count = counts.contains(cats[i].label) ? counts[cats[i].label] : 0;
        int barW  = static_cast<int>((static_cast<qreal>(count) / maxVal) * maxBarW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + 12, y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter,
                   cats[i].label);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(cats[i].color);
        QPainterPath bar;
        bar.addRoundedRect(area.x() + 12 + labelW + 8, y + 2, qMax(barW, 2), barH - 4, 3, 3);
        p.drawPath(bar);

        // Count badge
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(area.x() + 12 + labelW + 12 + barW, y + 2, 30, barH - 4,
                   Qt::AlignVCenter, QString::number(count));
    }
}

// ── stats boxes ───────────────────────────────────────────────────────────

void PaperNoveltyRadar2::drawStats(QPainter& p, const QRect& area) {
    // Card background
    QPainterPath card;
    card.addRoundedRect(area, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawPath(card);
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(card);

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(area.x() + 12, area.y() + 4, area.width() - 24, 22,
               Qt::AlignLeft | Qt::AlignVCenter, "Summary");

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Papers",       QString::number(entries_.size()),  QColor(59, 130, 246)},   // #3b82f6
        {"Breakthroughs",QString::number(breakthroughCount()), QColor(22, 163, 74)}, // #16a34a
        {"Avg Score",    QString::number(avgScore(), 'f', 1) + "/10", QColor(217, 119, 6)}, // #d97706
        {"Citations",    QString::number(totalCitations()), QColor(124, 58, 237)}    // #7c3aed
    };

    int boxH   = qMin(44, (area.height() - 50) / 4);
    int innerX = area.x() + 12;
    int innerW = area.width() - 24;

    for (int i = 0; i < stats.size(); ++i) {
        int y = area.y() + 32 + i * (boxH + 8);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath pill;
        pill.addRoundedRect(innerX, y, innerW, boxH, 6, 6);
        p.drawPath(pill);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 15, QFont::Bold));
        p.drawText(innerX + 10, y + 4, innerW - 20, boxH / 2,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(innerX + 10, y + boxH / 2, innerW - 20, boxH / 2 - 4,
                   Qt::AlignVCenter, stats[i].label);
    }
}
