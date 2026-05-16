#include "tools/PaperDnsLookup2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

// ── construction ──────────────────────────────────────────────────────────

PaperDnsLookup2::PaperDnsLookup2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DnsLookup2")
{
    setupUI();
    loadSettings();
}

void PaperDnsLookup2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Top toolbar row
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Internal", "External", "CDN", "Cloud", "Local"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 100px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter domain to resolve...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    lookupBtn_ = new QPushButton("Lookup");
    lookupBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(lookupBtn_, &QPushButton::clicked, this, &PaperDnsLookup2::onLookup);
    toolbar->addWidget(lookupBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 4px 12px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDnsLookup2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Paper DNS Lookup 2 — ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 2px 4px;");
    mainLayout->addWidget(infoLabel_);

    // Canvas area for custom painting
    mainLayout->addStretch(1);

    setMinimumSize(700, 560);

    // ── Seed 8 demo entries ───────────────────────────────────────────────
    if (entries_.isEmpty()) {
        struct Seed { QString domain; QString category; QString record;
                      qreal latency; int queries; bool resolved; };
        Seed seeds[] = {
            {"api.paperlab.io",     "External", "A",      12.4,  47, true },
            {"cdn.papersync.dev",   "CDN",      "CNAME",   8.1, 132, true },
            {"db.research.net",     "Internal", "AAAA",   34.7,  28, true },
            {"mail.university.edu", "Cloud",    "MX",    187.3,  15, true },
            {"auth.internal.corp",  "Local",    "A",       3.2,  89, true },
            {"portal.thesis.org",   "External", "TXT",   245.6,   9, false},
            {"backup.archive.edu",  "Cloud",    "AAAA",   98.5,  34, true },
            {"relay.internal.corp", "CDN",      "A",      22.0,  56, true },
        };

        // Palette color map
        QMap<QString, QColor> catColor;
        catColor["Internal"] = QColor(59, 130, 246);    // #3b82f6
        catColor["External"] = QColor(22, 163, 74);     // #16a34a
        catColor["CDN"]      = QColor(217, 119, 6);     // #d97706
        catColor["Cloud"]    = QColor(220, 38, 38);     // #dc2626
        catColor["Local"]    = QColor(124, 58, 237);    // #7c3aed

        for (int i = 0; i < 8; ++i) {
            const auto& s = seeds[i];
            DnsLookup2Entry e;
            e.id       = i + 1;
            e.domain   = s.domain;
            e.category = s.category;
            e.record   = s.record;
            e.latency  = s.latency;
            e.queries  = s.queries;
            e.resolved = s.resolved;
            e.color    = catColor.value(s.category, QColor(107, 114, 128));
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

// ── paint ─────────────────────────────────────────────────────────────────

void PaperDnsLookup2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    int toolbarH = 90;
    int canvasY  = toolbarH;
    int w = width() - 16;
    int h = height() - canvasY - 8;

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(QRect(8, canvasY, w, h), Qt::AlignCenter,
                   "No DNS entries — enter a domain and click Lookup");
        return;
    }

    // Top 55% : lookup view
    int topH = h * 55 / 100;
    drawLookupView(p, QRect(8, canvasY, w, topH));

    // Bottom split: left = category chart, right = stats
    int bottomY = canvasY + topH + 6;
    int bottomH = h - topH - 6;
    int halfW   = w / 2;

    drawCategoryChart(p, QRect(8, bottomY, halfW - 4, bottomH));
    drawStats(p, QRect(8 + halfW + 4, bottomY, halfW - 4, bottomH));
}

// ── drawLookupView ────────────────────────────────────────────────────────
//  Domain cards with record type badge, latency bar (green <50ms,
//  amber <200ms, red >=200ms), query count, resolved indicator.

void PaperDnsLookup2::drawLookupView(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(area.x() + 4, area.y() + 18, "DNS Lookup Results");

    // Background panel
    QPainterPath panelPath;
    panelPath.addRoundedRect(area.x(), area.y() + 26, area.width(),
                             area.height() - 26, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawPath(panelPath);

    // Filter by category
    QString filter = categoryCombo_->currentText();

    // Collect visible entries
    QList<const DnsLookup2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter) {
            visible.append(&e);
        }
    }

    int maxShow = qMin(8, static_cast<int>(visible.size()));
    if (maxShow == 0) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 10));
        p.drawText(area.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   "No entries for category: " + filter);
        return;
    }

    int availH = area.height() - 42;
    int itemH  = qMin(48, availH / qMax(maxShow, 1));
    int barMaxW = area.width() - 240;

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = *visible[i];
        int y = area.y() + 34 + i * (itemH + 4);
        int x = area.x() + 4;
        int rowW = area.width() - 8;

        // ── Row card background ───────────────────────────────────────
        QPainterPath rowPath;
        rowPath.addRoundedRect(x, y, rowW, itemH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(195));
        p.drawPath(rowPath);

        // Left accent bar
        QPainterPath accent;
        accent.addRoundedRect(x, y, 4, itemH, 2, 2);
        p.setBrush(e.color);
        p.drawPath(accent);

        // ── Domain name ───────────────────────────────────────────────
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 10, QFont::Bold));
        p.drawText(x + 14, y + 2, rowW / 3, 20, Qt::AlignVCenter,
                   e.domain.length() > 28 ? e.domain.left(26) + ".." : e.domain);

        // ── Record type badge ─────────────────────────────────────────
        int badgeW = 50;
        int badgeH = 18;
        int badgeX = x + rowW / 3 + 20;
        int badgeY = y + 3;

        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 9, 9);
        p.setBrush(e.color);
        p.drawPath(badgePath);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter, e.record);

        // ── Latency bar ───────────────────────────────────────────────
        int barH = 8;
        int barY = y + itemH - 14;
        int barX = x + 14;
        int filledW = static_cast<int>(
            (qBound(0.0, e.latency, 300.0) / 300.0) * barMaxW);

        // Determine bar color: green <50ms, amber <200ms, red >=200ms
        QColor barColor;
        if (e.latency < 50.0)
            barColor = QColor(22, 163, 74);    // #16a34a green
        else if (e.latency < 200.0)
            barColor = QColor(217, 119, 6);    // #d97706 amber
        else
            barColor = QColor(220, 38, 38);    // #dc2626 red

        // Track
        QPainterPath trackPath;
        trackPath.addRoundedRect(barX, barY, barMaxW, barH, 4, 4);
        p.setBrush(QColor(226, 232, 240));
        p.drawPath(trackPath);

        // Fill
        if (filledW > 0) {
            QPainterPath fillPath;
            fillPath.addRoundedRect(barX, barY, qMax(filledW, 4), barH, 4, 4);
            p.setBrush(barColor);
            p.drawPath(fillPath);
        }

        // Latency text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barMaxW + 6, barY + barH - 1,
                   QString::number(static_cast<int>(e.latency)) + "ms");

        // ── Query count ───────────────────────────────────────────────
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 14, y + 18, rowW - 110, 14, Qt::AlignVCenter,
                   e.category + "  |  Queries: " + QString::number(e.queries));

        // ── Resolved indicator ────────────────────────────────────────
        int dotR = 7;
        int dotX = x + rowW - 22;
        int dotY = y + itemH / 2 - dotR;

        QPainterPath dotPath;
        dotPath.addEllipse(dotX, dotY, dotR * 2, dotR * 2);
        p.setPen(Qt::NoPen);
        p.setBrush(e.resolved ? QColor(22, 163, 74)     // #16a34a green
                              : QColor(220, 38, 38));    // #dc2626 red
        p.drawPath(dotPath);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(dotX, dotY, dotR * 2, dotR * 2, Qt::AlignCenter,
                   e.resolved ? QString::fromUtf8("✓")   // checkmark
                              : QString::fromUtf8("✗")); // cross
    }
}

// ── drawCategoryChart ─────────────────────────────────────────────────────
//  Horizontal bar chart of entry counts per category.

void PaperDnsLookup2::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 4, area.y() + 16, "Category Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    // Background panel
    QPainterPath panelPath;
    panelPath.addRoundedRect(area.x(), area.y() + 22, area.width(),
                             area.height() - 22, 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawPath(panelPath);

    // Palette per category
    QMap<QString, QColor> catColor;
    catColor["Internal"] = QColor(59, 130, 246);    // #3b82f6
    catColor["External"] = QColor(22, 163, 74);     // #16a34a
    catColor["CDN"]      = QColor(217, 119, 6);     // #d97706
    catColor["Cloud"]    = QColor(220, 38, 38);     // #dc2626
    catColor["Local"]    = QColor(124, 58, 237);    // #7c3aed

    int maxVal = 1;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxVal = qMax(maxVal, it.value());

    int nCats     = counts.size();
    int listStart = area.y() + 32;
    int barH      = qMin(28, (area.height() - 44) / qMax(nCats, 1));
    int maxBarW   = area.width() - 130;

    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y     = listStart + idx * (barH + 6);
        int count = it.value();
        int barW  = static_cast<int>(
            (static_cast<qreal>(count) / maxVal) * maxBarW);
        QColor color = catColor.value(it.key(), QColor(107, 114, 128));

        // Category label (right-aligned)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(area.x() + 6, y, 88, barH,
                   Qt::AlignVCenter | Qt::AlignRight, it.key());

        // Horizontal bar
        if (barW > 0) {
            QPainterPath barPath;
            barPath.addRoundedRect(area.x() + 98, y + 3,
                                   qMax(barW, 4), barH - 6, 3, 3);
            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawPath(barPath);
        }

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + 104 + qMax(barW, 0), y, 28, barH,
                   Qt::AlignVCenter, QString::number(count));

        ++idx;
    }
}

// ── drawStats ─────────────────────────────────────────────────────────────
//  Four stat boxes: Total, Resolved, Avg Latency, Categories.

void PaperDnsLookup2::drawStats(QPainter& p, const QRect& area) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 4, area.y() + 16, "Statistics");

    // Background panel
    QPainterPath panelPath;
    panelPath.addRoundedRect(area.x(), area.y() + 22, area.width(),
                             area.height() - 22, 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawPath(panelPath);

    // Compute stat values
    int total    = entries_.size();
    int resolved = resolvedCount();
    qreal avg    = avgLatency();
    int cats     = categoryCounts().size();

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };
    QList<Stat> stats = {
        {"Total Entries",  QString::number(total),              QColor(59, 130, 246)},   // #3b82f6
        {"Resolved",       QString::number(resolved),           QColor(22, 163, 74)},    // #16a34a
        {"Avg Latency",    QString::number(avg, 'f', 1) + "ms", QColor(217, 119, 6)},   // #d97706
        {"Categories",     QString::number(cats),               QColor(124, 58, 237)},   // #7c3aed
    };

    int boxH    = qMin(48, (area.height() - 38) / qMax(stats.size(), 1));
    int boxW    = area.width() - 16;
    int startX  = area.x() + 8;
    int startY  = area.y() + 30;

    for (int i = 0; i < stats.size(); ++i) {
        int y = startY + i * (boxH + 5);

        // Box background
        QPainterPath boxPath;
        boxPath.addRoundedRect(startX, y, boxW, boxH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        p.drawPath(boxPath);

        // Left accent strip
        QPainterPath accentPath;
        accentPath.addRoundedRect(startX, y, 4, boxH, 2, 2);
        p.setBrush(stats[i].color);
        p.drawPath(accentPath);

        // Value (large)
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 15, QFont::Bold));
        p.drawText(startX + 14, y + 2, boxW - 20, 24,
                   Qt::AlignVCenter, stats[i].value);

        // Label (smaller, below value)
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(startX + 14, y + 26, boxW - 20, 18,
                   Qt::AlignVCenter, stats[i].label);
    }
}

// ── actions ───────────────────────────────────────────────────────────────

void PaperDnsLookup2::onLookup() {
    QString domain = inputField_->text().trimmed();
    if (domain.isEmpty()) return;

    // Resolve category from combo
    QString category = categoryCombo_->currentText();
    if (category == "All") {
        QStringList cats = {"Internal", "External", "CDN", "Cloud", "Local"};
        category = cats[QRandomGenerator::global()->bounded(cats.size())];
    }

    // Record types
    QStringList recordTypes = {"A", "AAAA", "CNAME", "MX", "TXT"};
    QString record = recordTypes[QRandomGenerator::global()->bounded(recordTypes.size())];

    // Simulate latency
    qreal latency = 2.0 + QRandomGenerator::global()->boundedDouble() * 298.0;
    int queries   = 1 + QRandomGenerator::global()->bounded(200);
    bool resolved = QRandomGenerator::global()->bounded(10) < 8; // 80% success

    // Category-based color (palette)
    QMap<QString, QColor> catColor;
    catColor["Internal"] = QColor(59, 130, 246);
    catColor["External"] = QColor(22, 163, 74);
    catColor["CDN"]      = QColor(217, 119, 6);
    catColor["Cloud"]    = QColor(220, 38, 38);
    catColor["Local"]    = QColor(124, 58, 237);

    DnsLookup2Entry entry;
    entry.id       = entries_.size() + 1;
    entry.domain   = domain;
    entry.category = category;
    entry.record   = record;
    entry.latency  = latency;
    entry.queries  = queries;
    entry.resolved = resolved;
    entry.color    = catColor.value(category, QColor(107, 114, 128));

    addEntry(entry);
    inputField_->clear();
}

void PaperDnsLookup2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ── data helpers ──────────────────────────────────────────────────────────

void PaperDnsLookup2::addEntry(const DnsLookup2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dnsResolved(entry.id, entry.latency);
    update();
}

QList<DnsLookup2Entry> PaperDnsLookup2::entries() const {
    return entries_;
}

int PaperDnsLookup2::resolvedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.resolved) ++count;
    return count;
}

qreal PaperDnsLookup2::avgLatency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.latency;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperDnsLookup2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ── info label ────────────────────────────────────────────────────────────

void PaperDnsLookup2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Paper DNS Lookup 2 — ready");
        return;
    }
    infoLabel_->setText(
        QString("Entries: %1  |  Resolved: %2  |  Avg Latency: %3ms  |  Categories: %4")
            .arg(entries_.size())
            .arg(resolvedCount())
            .arg(avgLatency(), 0, 'f', 1)
            .arg(categoryCounts().size()));
}

// ── persistence ───────────────────────────────────────────────────────────

void PaperDnsLookup2::loadSettings() {
    settings_.beginGroup("DnsLookup2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DnsLookup2Entry e;
        e.id       = settings_.value("id").toInt();
        e.domain   = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.record   = settings_.value("record").toString();
        e.latency  = settings_.value("latency").toReal();
        e.queries  = settings_.value("queries").toInt();
        e.resolved = settings_.value("resolved").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperDnsLookup2::saveSettings() {
    settings_.beginGroup("DnsLookup2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("domain",   entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("record",   entries_[i].record);
        settings_.setValue("latency",  entries_[i].latency);
        settings_.setValue("queries",  entries_[i].queries);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
