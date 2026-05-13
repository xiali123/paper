#include "tools/PaperCertGuard2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

// ── construction ──────────────────────────────────────────────────────────

PaperCertGuard2::PaperCertGuard2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CertGuard2")
{
    setupUI();
    loadSettings();
}

void PaperCertGuard2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Top toolbar row
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Web", "API", "Database", "Email", "Internal"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 110px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter domain...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    scanBtn_ = new QPushButton("Scan");
    scanBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(scanBtn_, &QPushButton::clicked, this, &PaperCertGuard2::onScan);
    toolbar->addWidget(scanBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 4px 12px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCertGuard2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Paper CertGuard 2 — ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 2px 4px;");
    mainLayout->addWidget(infoLabel_);

    // Canvas area for custom painting
    mainLayout->addStretch(1);

    setMinimumSize(700, 560);

    // ── Seed 8 demo entries ───────────────────────────────────────────────
    if (entries_.isEmpty()) {
        struct Seed { QString domain; QString category; QString algorithm;
                      qreal strength; int certs; bool weak; };
        Seed seeds[] = {
            {"api.paperlab.io",    "Web",       "RSA-2048",    92.5, 3, false},
            {"db.research.net",    "Database",  "ECDSA-P256",  88.0, 2, false},
            {"mail.university.edu","Email",     "RSA-4096",    95.0, 5, false},
            {"auth.internal.corp", "Internal",  "RSA-2048",    45.0, 1, true},
            {"cdn.papersync.dev",  "API",       "Ed25519",     98.0, 4, false},
            {"portal.thesis.org",  "Web",       "ECDSA-P256",  72.0, 2, false},
            {"backup.archive.edu", "Database",  "RSA-2048",    38.5, 1, true},
            {"relay.internal.corp","Internal",  "Ed25519",     55.0, 2, false},
        };

        // Palette color map
        QMap<QString, QColor> catColor;
        catColor["Web"]      = QColor(59, 130, 246);   // #3b82f6
        catColor["API"]      = QColor(124, 58, 237);   // #7c3aed
        catColor["Database"] = QColor(22, 163, 74);    // #16a34a
        catColor["Email"]    = QColor(217, 119, 6);    // #d97706
        catColor["Internal"] = QColor(220, 38, 38);    // #dc2626

        for (int i = 0; i < 8; ++i) {
            const auto& s = seeds[i];
            CertGuard2Entry e;
            e.id        = i + 1;
            e.domain    = s.domain;
            e.category  = s.category;
            e.algorithm = s.algorithm;
            e.strength  = s.strength;
            e.certs     = s.certs;
            e.weak      = s.weak;
            e.color     = catColor.value(s.category, QColor(107, 114, 128));
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

// ── paint ─────────────────────────────────────────────────────────────────

void PaperCertGuard2::paintEvent(QPaintEvent*) {
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
                   "No certificates scanned — enter a domain and click Scan");
        return;
    }

    // Top 55% : guard view
    int topH = h * 55 / 100;
    drawGuardView(p, QRect(8, canvasY, w, topH));

    // Bottom split: left = category chart, right = stats
    int bottomY = canvasY + topH + 6;
    int bottomH = h - topH - 6;
    int halfW   = w / 2;

    drawCategoryChart(p, QRect(8, bottomY, halfW - 4, bottomH));
    drawStats(p, QRect(8 + halfW + 4, bottomY, halfW - 4, bottomH));
}

// ── drawGuardView ─────────────────────────────────────────────────────────
//  Domain cards with algorithm badge, strength bar (green>80 amber>50 red),
//  cert count, and weak warning.

void PaperCertGuard2::drawGuardView(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(area.x() + 4, area.y() + 18, "Certificate Guard View");

    // Background panel
    QPainterPath panelPath;
    panelPath.addRoundedRect(area.x(), area.y() + 26, area.width(),
                             area.height() - 26, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawPath(panelPath);

    int maxShow = qMin(static_cast<int>(entries_.size()), 10);
    int availH  = area.height() - 42;
    int itemH   = qMin(52, availH / qMax(maxShow, 1));
    int barMaxW = area.width() - 200;

    int shown = 0;
    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        int y = area.y() + 34 + shown * (itemH + 4);
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

        // ── Algorithm badge ───────────────────────────────────────────
        int badgeW = 80;
        int badgeH = 18;
        int badgeX = x + rowW - badgeW - 8;
        int badgeY = y + 3;

        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 9, 9);
        p.setBrush(e.color);
        p.drawPath(badgePath);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter,
                   e.algorithm);

        // ── Domain name ───────────────────────────────────────────────
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(x + 14, y + 2, rowW - 110, 18, Qt::AlignVCenter,
                   e.domain.left(34));

        // ── Category + cert count ─────────────────────────────────────
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 14, y + 18, rowW - 110, 14, Qt::AlignVCenter,
                   e.category + "  |  Certs: " + QString::number(e.certs));

        // ── Strength bar ──────────────────────────────────────────────
        int barH = 8;
        int barY = y + itemH - 14;
        int barX = x + 14;
        int filledW = static_cast<int>(
            (qBound(0.0, e.strength, 100.0) / 100.0) * barMaxW);

        // Determine bar color: green >80, amber >50, red
        QColor barColor;
        if (e.strength > 80.0)
            barColor = QColor(22, 163, 74);    // #16a34a green
        else if (e.strength > 50.0)
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

        // Strength percentage label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barMaxW + 6, barY + barH - 1,
                   QString::number(e.strength, 'f', 1) + "%");

        // ── Weak warning badge ────────────────────────────────────────
        if (e.weak) {
            int warnW = 48;
            int warnH = 16;
            int warnX = x + rowW - warnW - 8;
            int warnY = y + itemH - warnH - 4;

            QPainterPath warnPath;
            warnPath.addRoundedRect(warnX, warnY, warnW, warnH, 8, 8);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38));
            p.drawPath(warnPath);

            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(warnX, warnY, warnW, warnH, Qt::AlignCenter, "WEAK");
        }

        ++shown;
    }
}

// ── drawCategoryChart ─────────────────────────────────────────────────────
//  Horizontal bar chart of certificate counts per category.

void PaperCertGuard2::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 4, area.y() + 16, "Category Breakdown");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    // Background panel
    QPainterPath panelPath;
    panelPath.addRoundedRect(area.x(), area.y() + 22, area.width(),
                             area.height() - 22, 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawPath(panelPath);

    int maxVal = 1;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxVal = qMax(maxVal, it.value());

    // Palette per category
    QMap<QString, QColor> catColor;
    catColor["Web"]      = QColor(59, 130, 246);
    catColor["API"]      = QColor(124, 58, 237);
    catColor["Database"] = QColor(22, 163, 74);
    catColor["Email"]    = QColor(217, 119, 6);
    catColor["Internal"] = QColor(220, 38, 38);

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
        QPainterPath barPath;
        barPath.addRoundedRect(area.x() + 98, y + 3,
                               qMax(barW, 4), barH - 6, 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(barPath);

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + 104 + barW, y, 28, barH,
                   Qt::AlignVCenter, QString::number(count));

        ++idx;
    }
}

// ── drawStats ─────────────────────────────────────────────────────────────
//  Four stat boxes: Total, Avg Strength, Weak Count, Strong Count.

void PaperCertGuard2::drawStats(QPainter& p, const QRect& area) {
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
    int total   = entries_.size();
    qreal avgS  = avgStrength();
    int weak    = weakCount();
    int strong  = 0;
    for (const auto& e : entries_)
        if (e.strength > 80.0) ++strong;

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };
    QList<Stat> stats = {
        {"Total Domains",    QString::number(total),            QColor(59, 130, 246)},
        {"Avg Strength",     QString::number(avgS, 'f', 1) + "%", QColor(217, 119, 6)},
        {"Weak Certificates", QString::number(weak),            QColor(220, 38, 38)},
        {"Strong (>80%)",    QString::number(strong),           QColor(22, 163, 74)},
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

void PaperCertGuard2::onScan() {
    QString domain = inputField_->text().trimmed();
    if (domain.isEmpty()) return;

    // Resolve category from combo
    QString category = categoryCombo_->currentText();
    if (category == "All")
        category = "Web";

    // Pick an algorithm at random from the four supported
    QStringList algos = {"RSA-2048", "ECDSA-P256", "RSA-4096", "Ed25519"};
    QString algo = algos.at(QRandomGenerator::global()->bounded(algos.size()));

    // Simulate strength score
    qreal strength = static_cast<qreal>(QRandomGenerator::global()->bounded(200, 1001)) / 10.0;
    strength = qBound(10.0, strength, 100.0);
    int certs  = QRandomGenerator::global()->bounded(1, 7);
    bool weak  = strength <= 50.0;

    // Category-based color (palette)
    QMap<QString, QColor> catColor;
    catColor["Web"]      = QColor(59, 130, 246);
    catColor["API"]      = QColor(124, 58, 237);
    catColor["Database"] = QColor(22, 163, 74);
    catColor["Email"]    = QColor(217, 119, 6);
    catColor["Internal"] = QColor(220, 38, 38);

    CertGuard2Entry entry;
    entry.id        = entries_.size() + 1;
    entry.domain    = domain;
    entry.category  = category;
    entry.algorithm = algo;
    entry.strength  = strength;
    entry.certs     = certs;
    entry.weak      = weak;
    entry.color     = catColor.value(category, QColor(107, 114, 128));

    addEntry(entry);
    inputField_->clear();
}

void PaperCertGuard2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ── data helpers ──────────────────────────────────────────────────────────

void PaperCertGuard2::addEntry(const CertGuard2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit certScanned(entry.id, entry.strength);
    update();
}

QList<CertGuard2Entry> PaperCertGuard2::entries() const {
    return entries_;
}

int PaperCertGuard2::weakCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.weak) ++count;
    return count;
}

qreal PaperCertGuard2::avgStrength() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperCertGuard2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ── info label ────────────────────────────────────────────────────────────

void PaperCertGuard2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Paper CertGuard 2 — ready");
        return;
    }
    infoLabel_->setText(
        QString("Domains: %1  |  Avg Strength: %2%  |  Weak: %3")
            .arg(entries_.size())
            .arg(avgStrength(), 0, 'f', 1)
            .arg(weakCount()));
}

// ── persistence ───────────────────────────────────────────────────────────

void PaperCertGuard2::loadSettings() {
    settings_.beginGroup("CertGuard2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CertGuard2Entry e;
        e.id        = settings_.value("id").toInt();
        e.domain    = settings_.value("domain").toString();
        e.category  = settings_.value("category").toString();
        e.algorithm = settings_.value("algorithm").toString();
        e.strength  = settings_.value("strength").toReal();
        e.certs     = settings_.value("certs").toInt();
        e.weak      = settings_.value("weak").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperCertGuard2::saveSettings() {
    settings_.beginGroup("CertGuard2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",        entries_[i].id);
        settings_.setValue("domain",    entries_[i].domain);
        settings_.setValue("category",  entries_[i].category);
        settings_.setValue("algorithm", entries_[i].algorithm);
        settings_.setValue("strength",  entries_[i].strength);
        settings_.setValue("certs",     entries_[i].certs);
        settings_.setValue("weak",      entries_[i].weak);
        settings_.setValue("color",     entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
