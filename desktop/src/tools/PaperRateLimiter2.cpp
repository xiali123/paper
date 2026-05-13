#include "tools/PaperRateLimiter2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

// ── construction ──────────────────────────────────────────────────────────

PaperRateLimiter2::PaperRateLimiter2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RateLimiter2")
{
    setupUI();
    loadSettings();
}

void PaperRateLimiter2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Toolbar row
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All Categories", "API", "Search", "Upload", "Download", "Auth"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 120px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter endpoint...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperRateLimiter2::onCheck);
    toolbar->addWidget(inputField_, 1);

    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperRateLimiter2::onCheck);
    toolbar->addWidget(checkBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #dc2626; "
        "border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRateLimiter2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Rate Limiter 2 ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(780, 580);
}

// ── data helpers ──────────────────────────────────────────────────────────

void PaperRateLimiter2::addEntry(const RateLimiter2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<RateLimiter2Entry> PaperRateLimiter2::entries() const {
    return entries_;
}

int PaperRateLimiter2::throttledCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.throttled) ++c;
    return c;
}

qreal PaperRateLimiter2::avgRate() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.rate;
    return sum / entries_.size();
}

QMap<QString, int> PaperRateLimiter2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ── slots ─────────────────────────────────────────────────────────────────

void PaperRateLimiter2::onCheck() {
    static const QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    struct SeedRow {
        QString endpoint;
        QString category;
        QString method;
        qreal rate;
        int requests;
        bool throttled;
    };

    SeedRow seeds[] = {
        {"/api/papers",      "API",      "GET",    0.92, 245, false},
        {"/search/query",    "Search",   "POST",   0.78, 189, false},
        {"/upload/pdf",      "Upload",   "PUT",    1.15, 312, true },
        {"/download/batch",  "Download", "GET",    0.65, 178, false},
        {"/auth/token",      "Auth",     "POST",   0.45,  67, false},
        {"/api/citations",   "API",      "GET",    1.28, 401, true },
        {"/search/suggest",  "Search",   "GET",    0.88, 203, false},
        {"/upload/figure",   "Upload",   "POST",   1.05, 278, true },
    };

    QString baseEndpoint = inputField_->text().trimmed();

    for (int i = 0; i < 8; ++i) {
        RateLimiter2Entry e;
        e.id        = entries_.size() + 1;
        e.endpoint  = baseEndpoint.isEmpty() ? seeds[i].endpoint : baseEndpoint;
        e.category  = seeds[i].category;
        e.method    = seeds[i].method;
        e.rate      = seeds[i].rate;
        e.requests  = seeds[i].requests;
        e.throttled = seeds[i].throttled;
        e.color     = palette[i % 5];
        addEntry(e);
        if (e.throttled)
            emit rateChecked(e.id, e.rate);
    }

    inputField_->clear();
}

void PaperRateLimiter2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ── persistence ───────────────────────────────────────────────────────────

void PaperRateLimiter2::loadSettings() {
    settings_.beginGroup("RateLimiter2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RateLimiter2Entry e;
        e.id        = settings_.value("id").toInt();
        e.endpoint  = settings_.value("endpoint").toString();
        e.category  = settings_.value("category").toString();
        e.method    = settings_.value("method").toString();
        e.rate      = settings_.value("rate").toDouble();
        e.requests  = settings_.value("requests").toInt();
        e.throttled = settings_.value("throttled").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperRateLimiter2::saveSettings() {
    settings_.beginGroup("RateLimiter2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",        entries_[i].id);
        settings_.setValue("endpoint",  entries_[i].endpoint);
        settings_.setValue("category",  entries_[i].category);
        settings_.setValue("method",    entries_[i].method);
        settings_.setValue("rate",      entries_[i].rate);
        settings_.setValue("requests",  entries_[i].requests);
        settings_.setValue("throttled", entries_[i].throttled);
        settings_.setValue("color",     entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

// ── info label ────────────────────────────────────────────────────────────

void PaperRateLimiter2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Rate Limiter 2 ready");
        return;
    }
    int totalRequests = 0;
    for (const auto& e : entries_) totalRequests += e.requests;
    infoLabel_->setText(QString("Entries: %1 | Throttled: %2 | Avg Rate: %3 | Total Requests: %4")
        .arg(entries_.size())
        .arg(throttledCount())
        .arg(avgRate(), 0, 'f', 2)
        .arg(totalRequests));
}

// ── painting ──────────────────────────────────────────────────────────────

void PaperRateLimiter2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 0, width() - 40, 28, Qt::AlignLeft | Qt::AlignVCenter,
               "Rate Limiter 2");

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No entries yet - click Check");
        return;
    }

    int w = width();
    int h = height();
    int toolbarH = 70;

    // Layout: limiter view left half, category chart top-right, stats bottom-right
    int leftW   = (w - 30) / 2;
    int rightW  = leftW;
    int topH    = (h - toolbarH) * 55 / 100;
    int bottomH = (h - toolbarH) - topH - 10;

    drawLimiterView(p, QRect(10, toolbarH, leftW, h - toolbarH - 10));
    drawCategoryChart(p, QRect(20 + leftW, toolbarH, rightW, topH));
    drawStats(p, QRect(20 + leftW, toolbarH + topH + 5, rightW, bottomH));
}

// ── limiter view ──────────────────────────────────────────────────────────

void PaperRateLimiter2::drawLimiterView(QPainter& p, const QRect& area) {
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
               Qt::AlignLeft | Qt::AlignVCenter, "Rate Monitor");

    // Filter by category combo
    QString catFilter = categoryCombo_->currentText();

    QList<const RateLimiter2Entry*> visible;
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
        QColor bgColor = e.throttled ? QColor(254, 242, 242) : QColor(248, 250, 252);
        p.setBrush(bgColor);
        QPainterPath bg;
        bg.addRoundedRect(leftX + 18, y + 2, innerW - 18, itemH - 4, 4, 4);
        p.drawPath(bg);

        // Color accent bar on left of entry
        p.setBrush(e.color);
        QPainterPath accent;
        accent.addRoundedRect(leftX + 18, y + 2, 3, itemH - 4, 1, 1);
        p.drawPath(accent);

        // Endpoint name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString endpointLabel = e.endpoint;
        if (e.throttled) endpointLabel += "  [!]";
        p.drawText(leftX + 28, y + 3, innerW / 2 - 20, (itemH - 4) / 2,
                   Qt::AlignVCenter, endpointLabel);

        // Method badge
        p.setPen(Qt::NoPen);
        QColor badgeBg = e.color.lighter(175);
        p.setBrush(badgeBg);
        int badgeW = 40;
        int badgeH = 13;
        int badgeY = y + itemH / 2 - badgeH / 2 - 1;
        QPainterPath badge;
        badge.addRoundedRect(leftX + 28, badgeY, badgeW, badgeH, 3, 3);
        p.drawPath(badge);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(leftX + 28, badgeY, badgeW, badgeH, Qt::AlignCenter, e.method);

        // Rate bar
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

        // Bar fill: red if rate > 1.0 (over limit), amber > 0.7, green otherwise
        qreal normalizedRate = qMin(e.rate / 2.0, 1.0);
        int barW = static_cast<int>(normalizedRate * barMaxW);
        QColor rateColor;
        if (e.rate > 1.0)
            rateColor = QColor(220, 38, 38);   // #dc2626
        else if (e.rate > 0.7)
            rateColor = QColor(217, 119, 6);   // #d97706
        else
            rateColor = QColor(22, 163, 74);   // #16a34a
        p.setBrush(rateColor);
        QPainterPath barFill;
        barFill.addRoundedRect(barX, barY, qMax(barW, 2), barH, 3, 3);
        p.drawPath(barFill);

        // Rate value text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barMaxW + 4, barY + barH,
                   QString::number(e.rate, 'f', 2) + " r/s");

        // Request count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + itemH - 6, barMaxW, 10,
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1 reqs").arg(e.requests));

        // Throttled warning icon
        if (e.throttled) {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(leftX + innerW - 20, y + itemH / 2 + 4, "!");
        }
    }
}

// ── horizontal bar chart ──────────────────────────────────────────────────

void PaperRateLimiter2::drawCategoryChart(QPainter& p, const QRect& area) {
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
               Qt::AlignLeft | Qt::AlignVCenter, "Category Distribution");

    struct CatInfo { QString label; QColor color; };
    CatInfo cats[] = {
        {"API",      QColor(59, 130, 246)},   // #3b82f6
        {"Search",   QColor(22, 163, 74)},    // #16a34a
        {"Upload",   QColor(217, 119, 6)},    // #d97706
        {"Download", QColor(220, 38, 38)},    // #dc2626
        {"Auth",     QColor(124, 58, 237)}    // #7c3aed
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

void PaperRateLimiter2::drawStats(QPainter& p, const QRect& area) {
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

    int totalRequests = 0;
    for (const auto& e : entries_) totalRequests += e.requests;

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Entries",  QString::number(entries_.size()),     QColor(59, 130, 246)},   // #3b82f6
        {"Throttled",      QString::number(throttledCount()),    QColor(22, 163, 74)},    // #16a34a
        {"Avg Rate",       QString::number(avgRate(), 'f', 2),   QColor(217, 119, 6)},    // #d97706
        {"Total Requests", QString::number(totalRequests),       QColor(124, 58, 237)}    // #7c3aed
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
