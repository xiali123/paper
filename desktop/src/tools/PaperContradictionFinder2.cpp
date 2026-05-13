#include "tools/PaperContradictionFinder2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <numeric>

// ── construction ──────────────────────────────────────────────────────────

PaperContradictionFinder2::PaperContradictionFinder2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContradictionFinder2")
{
    setupUI();
    loadSettings();
}

void PaperContradictionFinder2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Results", "Conclusions", "Data", "Interpretation"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { update(); });
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search sources...");
    toolbar->addWidget(inputField_, 1);

    scanBtn_ = new QPushButton("Scan");
    scanBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(scanBtn_, &QPushButton::clicked, this, &PaperContradictionFinder2::onScan);
    toolbar->addWidget(scanBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContradictionFinder2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();

    infoLabel_ = new QLabel();
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    toolbar->addWidget(infoLabel_);

    layout->addLayout(toolbar);
    setMinimumSize(680, 520);
}

// ── data helpers ──────────────────────────────────────────────────────────

void PaperContradictionFinder2::addEntry(const ContradictionFinder2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contradictionFound(entry.id, entry.conflict);
    update();
}

QList<ContradictionFinder2Entry> PaperContradictionFinder2::entries() const {
    return entries_;
}

int PaperContradictionFinder2::unresolvedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (!e.resolved) ++c;
    return c;
}

qreal PaperContradictionFinder2::avgConflict() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.conflict;
    return sum / entries_.size();
}

QMap<QString, int> PaperContradictionFinder2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ── slots ─────────────────────────────────────────────────────────────────

void PaperContradictionFinder2::onScan() {
    static const QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    struct SeedData {
        QString source1;
        QString category;
        QString source2;
        qreal conflict;
        int overlaps;
        bool resolved;
    };

    SeedData seeds[] = {
        {"Paper A", "Methodology",    "Paper B", 0.82, 5, false},
        {"Paper A", "Results",        "Paper C", 0.65, 3, false},
        {"Paper B", "Data",           "Paper D", 0.91, 7, false},
        {"Paper C", "Conclusions",    "Paper A", 0.48, 2, true },
        {"Paper D", "Interpretation", "Paper B", 0.73, 4, false},
        {"Paper A", "Data",           "Paper C", 0.55, 3, true },
        {"Paper B", "Methodology",    "Paper D", 0.38, 1, true },
        {"Paper C", "Results",        "Paper D", 0.87, 6, false},
    };

    for (int i = 0; i < 8; ++i) {
        ContradictionFinder2Entry e;
        e.id        = entries_.size() + 1;
        e.source1   = seeds[i].source1;
        e.category  = seeds[i].category;
        e.source2   = seeds[i].source2;
        e.conflict  = seeds[i].conflict;
        e.overlaps  = seeds[i].overlaps;
        e.resolved  = seeds[i].resolved;
        e.color     = palette[i % 5];
        addEntry(e);
    }
}

void PaperContradictionFinder2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("No contradictions found");
    update();
}

// ── painting ──────────────────────────────────────────────────────────────

void PaperContradictionFinder2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Contradiction Finder");

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Click Scan to detect contradictions between sources");
        return;
    }

    int w = width(), h = height();
    int contentTop = 45;

    int viewW  = static_cast<int>(w * 0.6) - 25;
    int chartX = static_cast<int>(w * 0.6) + 5;
    int chartW = static_cast<int>(w * 0.4) - 25;
    int topH   = static_cast<int>(h * 0.75) - contentTop;
    int statsY = static_cast<int>(h * 0.75) + 5;
    int statsH = h - statsY - 15;

    drawFinderView(p, QRect(15, contentTop, viewW, topH));
    drawCategoryChart(p, QRect(chartX, contentTop, chartW, topH));
    drawStats(p, QRect(15, statsY, w - 30, statsH));
}

// ── contradiction pair cards ──────────────────────────────────────────────

void PaperContradictionFinder2::drawFinderView(QPainter& p, const QRect& area) {
    QString filter = categoryCombo_->currentText();
    QString search = inputField_->text().trimmed().toLower();

    QList<const ContradictionFinder2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter != "All" && e.category != filter) continue;
        if (!search.isEmpty() &&
            !e.source1.toLower().contains(search) &&
            !e.source2.toLower().contains(search)) continue;
        visible.append(&e);
    }

    int cardH = qMin(52, (area.height() - 10) / qMax(1, visible.size()));
    int show  = qMin(visible.size(), (area.height() - 10) / qMax(1, cardH + 4));

    for (int i = 0; i < show; ++i) {
        const auto& e = *visible[visible.size() - 1 - i];
        int y = area.y() + i * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(area.x(), y, area.width(), cardH, 6, 6);

        // Source pair: "source1 vs source2"
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        QString pairLabel = e.source1 + "  vs  " + e.source2;
        p.drawText(area.x() + 10, y + 3, area.width() - 20, 18,
                   Qt::AlignVCenter, pairLabel);

        // Category badge
        QFontMetrics fmBadge(QFont("Arial", 8));
        int badgeW = fmBadge.horizontalAdvance(e.category) + 14;
        int badgeX = area.x() + 10;
        int badgeY = y + 22;

        QColor badgeBg = e.color;
        badgeBg.setAlpha(40);
        p.setBrush(badgeBg);
        p.drawRoundedRect(badgeX, badgeY, badgeW, 16, 3, 3);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 8));
        p.drawText(badgeX + 7, badgeY, badgeW - 14, 16, Qt::AlignVCenter, e.category);

        // Conflict bar
        int barX = badgeX + badgeW + 12;
        int barW = qMin(100, area.width() - badgeW - 140);
        int barH = 8;
        int barY = badgeY + 4;

        // Bar track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);

        // Bar fill: red > 0.7, amber > 0.4, green otherwise
        QColor conflictColor;
        if (e.conflict > 0.7)      conflictColor = QColor(220, 38, 38);   // #dc2626
        else if (e.conflict > 0.4) conflictColor = QColor(217, 119, 6);   // #d97706
        else                       conflictColor = QColor(22, 163, 74);   // #16a34a
        int fillW = static_cast<int>(e.conflict * barW);
        p.setBrush(conflictColor);
        p.drawRoundedRect(barX, barY, qMax(fillW, 2), barH, 3, 3);

        // Conflict value
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, barY + barH,
                   QString::number(e.conflict, 'f', 2));

        // Overlap count (right side)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 11, QFont::Bold));
        p.drawText(area.x() + area.width() - 60, y + 2, 30, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.overlaps));
        p.setFont(QFont("Arial", 7));
        p.setPen(QColor(100, 116, 139));
        p.drawText(area.x() + area.width() - 60, y + 20, 30, 14,
                   Qt::AlignVCenter | Qt::AlignRight, "overlaps");

        // Resolved / Active indicator
        if (e.resolved) {
            // Green check circle
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawEllipse(area.x() + area.width() - 22, y + cardH / 2 - 5, 10, 10);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(area.x() + area.width() - 22, y + cardH / 2 - 5, 10, 10,
                       Qt::AlignCenter, QString::fromUtf8("\342\234\223"));
        } else {
            // Red/amber active dot
            p.setPen(Qt::NoPen);
            p.setBrush(conflictColor);
            p.drawEllipse(area.x() + area.width() - 22, y + cardH / 2 - 5, 10, 10);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(area.x() + area.width() - 22, y + cardH / 2 - 5, 10, 10,
                       Qt::AlignCenter, "!");
        }
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(area, Qt::AlignCenter, "No matching contradictions");
    }
}

// ── stacked bar chart ─────────────────────────────────────────────────────

static int fmAdvance(const QFontMetrics& fm, const QString& text) {
    return fm.horizontalAdvance(text);
}

void PaperContradictionFinder2::drawCategoryChart(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Category Distribution");

    QStringList categories = {"Methodology", "Results", "Conclusions", "Data", "Interpretation"};
    QColor catColors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    // Count resolved / unresolved per category for stacking
    QMap<QString, int> resolvedCounts;
    QMap<QString, int> unresolvedCounts;
    auto counts = categoryCounts();
    for (const auto& e : entries_) {
        if (e.resolved) resolvedCounts[e.category]++;
        else            unresolvedCounts[e.category]++;
    }

    int chartTop  = area.y() + 22;
    int chartH    = area.height() - 50;
    int catCount  = categories.size();
    int barGroupW = qMin(50, (area.width() - 10) / catCount - 6);

    // Find max total for scaling
    int maxTotal = 1;
    for (const auto& cat : categories) {
        int total = counts.value(cat, 0);
        maxTotal = qMax(maxTotal, total);
    }

    for (int c = 0; c < catCount; ++c) {
        int x = area.x() + c * (barGroupW + 6) + 4;
        int barBottom = chartTop + chartH;

        int resolv  = resolvedCounts.value(categories[c], 0);
        int unresolv = unresolvedCounts.value(categories[c], 0);

        // Resolved segment (bottom, lighter shade)
        int resolvH = static_cast<int>(
            (static_cast<qreal>(resolv) / maxTotal) * chartH);
        if (resolvH > 0) {
            p.setPen(Qt::NoPen);
            p.setBrush(catColors[c].lighter(140));
            p.drawRoundedRect(x, barBottom - resolvH, barGroupW, resolvH, 2, 2);
        }

        // Unresolved segment (top, full color)
        int unresolvH = static_cast<int>(
            (static_cast<qreal>(unresolv) / maxTotal) * chartH);
        if (unresolvH > 0) {
            p.setPen(Qt::NoPen);
            p.setBrush(catColors[c]);
            p.drawRoundedRect(x, barBottom - resolvH - unresolvH, barGroupW, unresolvH, 2, 2);
        }

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 2, barBottom + 4, barGroupW + 4, 14,
                   Qt::AlignHCenter | Qt::AlignTop,
                   categories[c].left(5));
    }

    // Legend at bottom
    int legendY = chartTop + chartH + 20;
    int legendX = area.x();
    p.setFont(QFont("Arial", 7));

    // Unresolved legend item
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(59, 130, 246));
    p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);
    p.setPen(QColor(100, 116, 139));
    p.drawText(legendX + 13, legendY + 10, "Active");
    legendX += fmAdvance(p.fontMetrics(), "Active") + 22;

    // Resolved legend item
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(59, 130, 246).lighter(140));
    p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);
    p.setPen(QColor(100, 116, 139));
    p.drawText(legendX + 13, legendY + 10, "Resolved");
}

// ── stats boxes ───────────────────────────────────────────────────────────

void PaperContradictionFinder2::drawStats(QPainter& p, const QRect& area) {
    int totalOverlaps = std::accumulate(entries_.cbegin(), entries_.cend(), 0,
        [](int s, const ContradictionFinder2Entry& e) { return s + e.overlaps; });

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Pairs",     QString::number(entries_.size()),  QColor(59, 130, 246)},   // #3b82f6
        {"Unresolved",      QString::number(unresolvedCount()), QColor(220, 38, 38)},   // #dc2626
        {"Avg Conflict",    QString::number(avgConflict(), 'f', 2), QColor(217, 119, 6)},// #d97706
        {"Total Overlaps",  QString::number(totalOverlaps),    QColor(124, 58, 237)}    // #7c3aed
    };

    int boxW = qMin(160, (area.width() - 30) / 4);
    int boxH = qMin(48, area.height() - 4);

    for (int i = 0; i < stats.size(); ++i) {
        int x = area.x() + i * (boxW + 10);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, area.y(), boxW, boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, area.y() + 4, boxW - 20, 24,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 10, area.y() + 28, boxW - 20, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

// ── info label ────────────────────────────────────────────────────────────

void PaperContradictionFinder2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No contradictions found");
        return;
    }
    int totalOverlaps = std::accumulate(entries_.cbegin(), entries_.cend(), 0,
        [](int s, const ContradictionFinder2Entry& e) { return s + e.overlaps; });
    infoLabel_->setText(
        QString("%1 pairs | %2 unresolved | avg conflict %3 | %4 overlaps")
            .arg(entries_.size())
            .arg(unresolvedCount())
            .arg(avgConflict(), 0, 'f', 2)
            .arg(totalOverlaps));
}

// ── persistence ───────────────────────────────────────────────────────────

void PaperContradictionFinder2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContradictionFinder2Entry e;
        e.id       = settings_.value("id").toInt();
        e.source1  = settings_.value("source1").toString();
        e.category = settings_.value("category").toString();
        e.source2  = settings_.value("source2").toString();
        e.conflict = settings_.value("conflict").toDouble();
        e.overlaps = settings_.value("overlaps").toInt();
        e.resolved = settings_.value("resolved").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContradictionFinder2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("source1",  entries_[i].source1);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("source2",  entries_[i].source2);
        settings_.setValue("conflict", entries_[i].conflict);
        settings_.setValue("overlaps", entries_[i].overlaps);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
