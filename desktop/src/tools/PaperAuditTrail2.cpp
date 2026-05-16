#include "tools/PaperAuditTrail2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

// ── construction ──────────────────────────────────────────────────────────

PaperAuditTrail2::PaperAuditTrail2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AuditTrail2")
{
    setupUI();
    loadSettings();
}

void PaperAuditTrail2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Toolbar row
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All Categories", "Security", "Data", "System", "User", "Access"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 120px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter audit action...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperAuditTrail2::onAudit);
    toolbar->addWidget(inputField_, 1);

    auditBtn_ = new QPushButton("Audit");
    auditBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(auditBtn_, &QPushButton::clicked, this, &PaperAuditTrail2::onAudit);
    toolbar->addWidget(auditBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #dc2626; "
        "border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAuditTrail2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Audit Trail 2 ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(780, 580);
}

// ── data helpers ──────────────────────────────────────────────────────────

void PaperAuditTrail2::addEntry(const AuditTrail2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<AuditTrail2Entry> PaperAuditTrail2::entries() const {
    return entries_;
}

int PaperAuditTrail2::flaggedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.flagged) ++c;
    return c;
}

qreal PaperAuditTrail2::avgImpact() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.impact;
    return sum / entries_.size();
}

QMap<QString, int> PaperAuditTrail2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ── slots ─────────────────────────────────────────────────────────────────

void PaperAuditTrail2::onAudit() {
    static const QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    struct SeedRow {
        QString action;
        QString category;
        QString user;
        qreal impact;
        int events;
        bool flagged;
    };

    SeedRow seeds[] = {
        {"Data Export",       "Data",     "researcher", 0.85, 14, true },
        {"Config Change",     "System",   "admin",      0.62,  8, false},
        {"User Login",        "Access",   "reviewer",   0.23,  3, false},
        {"Permission Update", "Security", "admin",      0.78, 11, true },
        {"Data Export",       "Data",     "researcher", 0.45,  6, false},
        {"Config Change",     "User",     "admin",      0.91, 18, true },
        {"User Login",        "Access",   "researcher", 0.18,  2, false},
        {"Permission Update", "Security", "reviewer",   0.55,  7, false},
    };

    QString baseAction = inputField_->text().trimmed();

    for (int i = 0; i < 8; ++i) {
        AuditTrail2Entry e;
        e.id       = entries_.size() + 1;
        e.action   = baseAction.isEmpty() ? seeds[i].action : baseAction;
        e.category = seeds[i].category;
        e.user     = seeds[i].user;
        e.impact   = seeds[i].impact;
        e.events   = seeds[i].events;
        e.flagged  = seeds[i].flagged;
        e.color    = palette[i % 5];
        addEntry(e);
        if (e.flagged)
            emit eventFlagged(e.id, e.impact);
    }

    inputField_->clear();
}

void PaperAuditTrail2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ── persistence ───────────────────────────────────────────────────────────

void PaperAuditTrail2::loadSettings() {
    settings_.beginGroup("AuditTrail2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AuditTrail2Entry e;
        e.id       = settings_.value("id").toInt();
        e.action   = settings_.value("action").toString();
        e.category = settings_.value("category").toString();
        e.user     = settings_.value("user").toString();
        e.impact   = settings_.value("impact").toDouble();
        e.events   = settings_.value("events").toInt();
        e.flagged  = settings_.value("flagged").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperAuditTrail2::saveSettings() {
    settings_.beginGroup("AuditTrail2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("action",   entries_[i].action);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("user",     entries_[i].user);
        settings_.setValue("impact",   entries_[i].impact);
        settings_.setValue("events",   entries_[i].events);
        settings_.setValue("flagged",  entries_[i].flagged);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

// ── info label ────────────────────────────────────────────────────────────

void PaperAuditTrail2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Audit Trail 2 ready");
        return;
    }
    int totalEvents = 0;
    for (const auto& e : entries_) totalEvents += e.events;
    infoLabel_->setText(QString("Entries: %1 | Flagged: %2 | Avg Impact: %3 | Total Events: %4")
        .arg(entries_.size())
        .arg(flaggedCount())
        .arg(avgImpact(), 0, 'f', 2)
        .arg(totalEvents));
}

// ── painting ──────────────────────────────────────────────────────────────

void PaperAuditTrail2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 0, width() - 40, 28, Qt::AlignLeft | Qt::AlignVCenter,
               "Audit Trail 2");

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No entries yet - click Audit");
        return;
    }

    int w = width();
    int h = height();
    int toolbarH = 70;

    // Layout: trail view left half, category chart top-right, stats bottom-right
    int leftW   = (w - 30) / 2;
    int rightW  = leftW;
    int topH    = (h - toolbarH) * 55 / 100;
    int bottomH = (h - toolbarH) - topH - 10;

    drawTrailView(p, QRect(10, toolbarH, leftW, h - toolbarH - 10));
    drawCategoryChart(p, QRect(20 + leftW, toolbarH, rightW, topH));
    drawStats(p, QRect(20 + leftW, toolbarH + topH + 5, rightW, bottomH));
}

// ── timeline trail view ───────────────────────────────────────────────────

void PaperAuditTrail2::drawTrailView(QPainter& p, const QRect& area) {
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
               Qt::AlignLeft | Qt::AlignVCenter, "Trail Timeline");

    // Filter by category combo
    QString catFilter = categoryCombo_->currentText();

    QList<const AuditTrail2Entry*> visible;
    for (const auto& e : entries_) {
        if (catFilter != "All Categories" && e.category != catFilter) continue;
        visible.append(&e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(160, 174, 192));
        p.setFont(QFont("Arial", 9));
        p.drawText(area, Qt::AlignCenter, "No entries for current filter");
        return;
    }

    int show  = qMin(visible.size(), 20);
    int itemH = qMin(36, (area.height() - 40) / qMax(show, 1));
    int topY  = area.y() + 30;
    int leftX = area.x() + 12;
    int innerW = area.width() - 24;

    // Timeline vertical line
    int lineX = leftX + 8;
    p.setPen(QPen(QColor(226, 232, 240), 2));
    p.drawLine(lineX, topY + 6, lineX, topY + show * itemH - 6);

    for (int i = 0; i < show; ++i) {
        const auto& e = *visible[i];
        int y = topY + i * itemH;

        // Timeline dot
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(QPointF(lineX, y + itemH / 2), 5, 5);

        // Entry background
        QColor bgColor = e.flagged ? QColor(254, 242, 242) : QColor(248, 250, 252);
        p.setBrush(bgColor);
        QPainterPath bg;
        bg.addRoundedRect(leftX + 18, y + 2, innerW - 18, itemH - 4, 4, 4);
        p.drawPath(bg);

        // Color accent bar on left of entry
        p.setBrush(e.color);
        QPainterPath accent;
        accent.addRoundedRect(leftX + 18, y + 2, 3, itemH - 4, 1, 1);
        p.drawPath(accent);

        // Action name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString actionLabel = e.action;
        if (e.flagged) actionLabel += "  [!]";
        p.drawText(leftX + 28, y + 3, innerW / 2 - 20, (itemH - 4) / 2,
                   Qt::AlignVCenter, actionLabel);

        // User badge
        p.setPen(Qt::NoPen);
        QColor badgeBg = e.color.lighter(175);
        p.setBrush(badgeBg);
        int badgeW = 60;
        int badgeH = 13;
        int badgeY = y + itemH / 2 - badgeH / 2 - 1;
        QPainterPath badge;
        badge.addRoundedRect(leftX + 28, badgeY, badgeW, badgeH, 3, 3);
        p.drawPath(badge);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(leftX + 28, badgeY, badgeW, badgeH, Qt::AlignCenter, e.user);

        // Impact bar
        int barX     = leftX + innerW / 2 + 28;
        int barMaxW  = innerW / 3;
        int barH     = 8;
        int barY     = y + 5;

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        QPainterPath barBg;
        barBg.addRoundedRect(barX, barY, barMaxW, barH, 3, 3);
        p.drawPath(barBg);

        // Bar fill (red > 0.7, amber > 0.4, green otherwise)
        int barW = static_cast<int>(e.impact * barMaxW);
        QColor impactColor;
        if (e.impact > 0.7)
            impactColor = QColor(220, 38, 38);   // #dc2626
        else if (e.impact > 0.4)
            impactColor = QColor(217, 119, 6);   // #d97706
        else
            impactColor = QColor(22, 163, 74);   // #16a34a
        p.setBrush(impactColor);
        QPainterPath barFill;
        barFill.addRoundedRect(barX, barY, qMax(barW, 2), barH, 3, 3);
        p.drawPath(barFill);

        // Impact value text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barMaxW + 4, barY + barH,
                   QString::number(e.impact, 'f', 2));

        // Event count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + itemH - 6, barMaxW, 10,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1 events").arg(e.events));

        // Flagged warning icon
        if (e.flagged) {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(leftX + innerW - 20, y + itemH / 2 + 4, "!"); // warning
        }
    }
}

// ── horizontal bar chart ──────────────────────────────────────────────────

void PaperAuditTrail2::drawCategoryChart(QPainter& p, const QRect& area) {
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
               Qt::AlignLeft | Qt::AlignVCenter, "Action Distribution");

    struct CatInfo { QString label; QColor color; };
    CatInfo cats[] = {
        {"Security", QColor(59, 130, 246)},   // #3b82f6
        {"Data",     QColor(22, 163, 74)},    // #16a34a
        {"System",   QColor(217, 119, 6)},    // #d97706
        {"User",     QColor(220, 38, 38)},    // #dc2626
        {"Access",   QColor(124, 58, 237)}    // #7c3aed
    };

    auto counts = categoryCounts();
    int maxVal = 1;
    for (const auto& c : cats)
        maxVal = qMax(maxVal, counts.contains(c.label) ? counts[c.label] : 0);

    int chartTop = area.y() + 32;
    int barH     = qMin(20, (area.height() - 50) / 5);
    int maxBarW  = area.width() - 140;
    int labelW   = 56;

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

void PaperAuditTrail2::drawStats(QPainter& p, const QRect& area) {
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

    int totalEvents = 0;
    for (const auto& e : entries_) totalEvents += e.events;

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Events",  QString::number(entries_.size()),     QColor(59, 130, 246)},   // #3b82f6
        {"Flagged Count", QString::number(flaggedCount()),      QColor(22, 163, 74)},    // #16a34a
        {"Avg Impact",    QString::number(avgImpact(), 'f', 2), QColor(217, 119, 6)},    // #d97706
        {"Event Sum",     QString::number(totalEvents),         QColor(124, 58, 237)}    // #7c3aed
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
