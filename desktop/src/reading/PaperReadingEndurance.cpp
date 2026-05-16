#include "reading/PaperReadingEndurance.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QFont>
#include <QtMath>
#include <QCoreApplication>

static const QMap<QString, QColor> kCategoryColors = {
    {QStringLiteral("Long"),   QColor("#3b82f6")},
    {QStringLiteral("Medium"), QColor("#16a34a")},
    {QStringLiteral("Short"),  QColor("#7c3aed")},
    {QStringLiteral("Sprint"), QColor("#d97706")}
};

static QColor colorForCategory(const QString& category)
{
    auto it = kCategoryColors.constFind(category);
    if (it != kCategoryColors.constEnd())
        return it.value();
    return QColor("#6b7280");
}

// ---------- Construction ----------

PaperReadingEndurance::PaperReadingEndurance(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingEndurance")
    , trackBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingEndurance::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // --- Input row ---
    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("All"));
    categoryCombo_->addItem(tr("Long"));
    categoryCombo_->addItem(tr("Medium"));
    categoryCombo_->addItem(tr("Short"));
    categoryCombo_->addItem(tr("Sprint"));
    inputRow->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Enter session..."));
    inputField_->setMinimumWidth(220);
    inputRow->addWidget(inputField_);

    trackBtn_ = new QPushButton(tr("Track"), this);
    trackBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    inputRow->addWidget(trackBtn_);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}"
        "QPushButton:pressed{background:#991b1b;}");
    inputRow->addWidget(clearBtn_);

    inputRow->addStretch(1);

    root->addLayout(inputRow);

    // --- Info label ---
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("font-size:13px; color:#6b7280;");
    root->addWidget(infoLabel_);

    // --- Custom painted area stretches to fill remaining space ---
    root->addStretch(1);

    setMinimumSize(640, 480);

    connect(trackBtn_, &QPushButton::clicked, this, &PaperReadingEndurance::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingEndurance::onClear);
}

// ---------- Public API ----------

void PaperReadingEndurance::addEntry(const EnduranceEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<EnduranceEntry> PaperReadingEndurance::entries() const
{
    return entries_;
}

int PaperReadingEndurance::sustainedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.sustained)
            ++count;
    }
    return count;
}

qreal PaperReadingEndurance::avgStamina() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.stamina;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingEndurance::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ---------- Slots ----------

void PaperReadingEndurance::onTrack()
{
    const QString session = inputField_->text().trimmed();
    if (session.isEmpty())
        return;

    auto* rng = QRandomGenerator::global();

    EnduranceEntry entry;
    entry.id       = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.session  = session;
    entry.category = categoryCombo_->currentText();
    entry.material = QString("Material-%1").arg(entry.id);
    entry.stamina  = rng->bounded(1000) / 1000.0;  // 0.0 - 1.0
    entry.pages    = rng->bounded(1, 101);           // 1 - 100
    entry.sustained = (entry.stamina > 0.55);
    entry.color    = colorForCategory(entry.category);

    entries_.append(entry);

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();

    emit enduranceTracked(entry.id, entry.stamina);
}

void PaperReadingEndurance::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------- Painting ----------

void PaperReadingEndurance::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    // Reserve top area for controls (already handled by layout).
    const int controlH = (inputField_ ? inputField_->geometry().bottom() : 0)
                       + (infoLabel_  ? infoLabel_->geometry().height() : 0) + 24;
    const int drawTop = qMax(controlH, 80);

    const int drawH = h - drawTop - 10;

    // Top half: endurance bars view
    const QRect enduranceRect(10, drawTop, w - 20, drawH / 2 - 5);

    // Bottom half: category chart (left) + stats (right)
    const int bottomY = drawTop + drawH / 2 + 5;
    const int halfW = (w - 30) / 2;
    const QRect categoryRect(10, bottomY, halfW, drawH / 2 - 5);
    const QRect statsRect(20 + halfW, bottomY, halfW, drawH / 2 - 5);

    drawEnduranceView(p, enduranceRect);
    drawCategoryChart(p, categoryRect);
    drawStats(p, statsRect);
}

void PaperReadingEndurance::drawEnduranceView(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Reading Endurance"));

    if (entries_.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(12, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No endurance sessions tracked yet."));
        return;
    }

    const int leftMargin = 12;
    const int topMargin  = 38;
    const int bottomMargin = 8;
    const int rightMargin = 12;
    const int barGap = 5;

    const int usableW = rect.width() - leftMargin - rightMargin;
    const int usableH = rect.height() - topMargin - bottomMargin;

    const int maxVisible = qMax(1, usableW / 70);
    const int startIdx = qMax(0, entries_.size() - maxVisible);
    const int visibleCount = entries_.size() - startIdx;

    if (visibleCount <= 0)
        return;

    const int barW = qBound(20, (usableW - barGap * (visibleCount - 1)) / visibleCount, 56);
    const int totalBarW = visibleCount * barW + (visibleCount - 1) * barGap;
    const int offsetX = rect.x() + leftMargin + (usableW - totalBarW) / 2;

    QFont smallFont = p.font();
    smallFont.setBold(false);
    smallFont.setPointSize(7);
    p.setFont(smallFont);

    for (int vi = 0; vi < visibleCount; ++vi) {
        const int idx = startIdx + vi;
        const auto& entry = entries_.at(idx);

        const int x = offsetX + vi * (barW + barGap);
        const int y = rect.y() + topMargin;
        const int fullH = usableH;

        // Background bar (light)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        QPainterPath bgPath;
        bgPath.addRoundedRect(x, y, barW, fullH, 4, 4);
        p.drawPath(bgPath);

        // Filled portion proportional to stamina
        const int fillH = static_cast<int>(fullH * qBound(0.0, entry.stamina, 1.0));
        if (fillH > 0) {
            QColor barColor = entry.color;
            barColor.setAlpha(200);
            p.setBrush(barColor);
            QPainterPath fillPath;
            fillPath.addRoundedRect(x, y + fullH - fillH, barW, fillH, 4, 4);
            p.drawPath(fillPath);
        }

        // Session name (rotated or truncated)
        p.setPen(QColor("#374151"));
        QFontMetrics fm(smallFont);
        const QString shortName = fm.elidedText(entry.session, Qt::ElideRight, barW);
        p.drawText(QRect(x, y + fullH + 2, barW, 14),
                   Qt::AlignCenter | Qt::AlignTop, shortName);

        // Pages label inside bar at top of filled region
        if (fillH > 16) {
            p.setPen(QColor("#ffffff"));
            QFont boldSmall = smallFont;
            boldSmall.setBold(true);
            p.setFont(boldSmall);
            p.drawText(QRect(x, y + fullH - fillH, barW, 16),
                       Qt::AlignHCenter | Qt::AlignTop,
                       QString::number(entry.pages) + QStringLiteral("p"));
            p.setFont(smallFont);
        }

        // Sustained badge (small green circle)
        if (entry.sustained) {
            const int badgeR = 5;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#16a34a"));
            p.drawEllipse(x + barW - badgeR - 2, y + 2, badgeR * 2, badgeR * 2);
        }

        // Stamina percentage on top of bar
        p.setPen(QColor("#64748b"));
        p.drawText(QRect(x, y - 14, barW, 14),
                   Qt::AlignCenter | Qt::AlignBottom,
                   QString::number(static_cast<int>(entry.stamina * 100)) + QStringLiteral("%"));
    }
}

void PaperReadingEndurance::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Category Breakdown"));

    const auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(12, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No category data yet."));
        return;
    }

    // Compute total for donut slices
    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();
    if (total == 0)
        return;

    const int cx = rect.x() + rect.width() / 2;
    const int cy = rect.y() + rect.height() / 2 + 8;
    const int outerR = qMin(rect.width(), rect.height()) / 2 - 30;
    const int innerR = static_cast<int>(outerR * 0.55);

    qreal angle = 0.0;
    QFont labelFont = p.font();
    labelFont.setBold(false);
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        const qreal span = 360.0 * static_cast<qreal>(it.value()) / static_cast<qreal>(total);
        const QColor c = colorForCategory(it.key());

        p.setPen(Qt::NoPen);
        p.setBrush(c);

        QPainterPath slice;
        const qreal startAngle16 = angle * 16;
        const qreal spanAngle16 = span * 16;
        slice.moveTo(cx, cy);
        slice.arcTo(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                    angle, span);
        slice.lineTo(cx, cy);
        slice.closeSubpath();
        p.drawPath(slice);

        // Label on the slice midpoint
        const qreal midAngle = qDegreesToRadians(angle + span / 2.0);
        const qreal labelR = (outerR + innerR) / 2.0;
        const qreal lx = cx + labelR * qCos(midAngle);
        const qreal ly = cy - labelR * qSin(midAngle);

        p.setPen(QColor("#ffffff"));
        p.drawText(QRect(static_cast<int>(lx) - 30, static_cast<int>(ly) - 8, 60, 16),
                   Qt::AlignCenter,
                   it.key() + QStringLiteral("\n") + QString::number(it.value()));

        angle += span;
    }

    // Inner circle (donut hole)
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text
    p.setPen(QColor("#1e293b"));
    QFont centerFont = p.font();
    centerFont.setBold(true);
    centerFont.setPointSize(12);
    p.setFont(centerFont);
    p.drawText(QRect(cx - innerR, cy - 12, innerR * 2, 24),
               Qt::AlignCenter, QString::number(total));
    centerFont.setBold(false);
    centerFont.setPointSize(8);
    p.setFont(centerFont);
    p.setPen(QColor("#64748b"));
    p.drawText(QRect(cx - innerR, cy + 8, innerR * 2, 16),
               Qt::AlignCenter, tr("sessions"));
}

void PaperReadingEndurance::drawStats(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Statistics"));

    QFont body = p.font();
    body.setBold(false);
    body.setPointSize(10);
    p.setFont(body);

    const int totalSessions = entries_.size();
    const int sustained = sustainedCount();
    const qreal avg = avgStamina();

    const int totalPages = [&]() {
        int s = 0;
        for (const auto& e : entries_)
            s += e.pages;
        return s;
    }();

    const int left = rect.x() + 14;
    int y = rect.y() + 40;
    const int lineH = 22;

    auto drawLine = [&](const QString& label, const QString& value) {
        p.setPen(QColor("#64748b"));
        p.drawText(left, y, label);
        p.setPen(QColor("#0f172a"));
        p.drawText(left + 120, y, value);
        y += lineH;
    };

    drawLine(tr("Total sessions:"), QString::number(totalSessions));
    drawLine(tr("Sustained:"),      QString::number(sustained));
    drawLine(tr("Avg stamina:"),    QString::number(avg, 'f', 2));
    drawLine(tr("Total pages:"),    QString::number(totalPages));

    if (totalSessions > 0) {
        const qreal sustainRate = static_cast<qreal>(sustained) / static_cast<qreal>(totalSessions) * 100.0;
        drawLine(tr("Sustain rate:"), QString::number(sustainRate, 'f', 1) + QStringLiteral("%"));
    }

    // Mini stamina indicator bar
    if (totalSessions > 0) {
        y += 10;
        const int barW = rect.width() - 28;
        const int barH = 10;

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        QPainterPath bgBar;
        bgBar.addRoundedRect(left, y, barW, barH, 5, 5);
        p.drawPath(bgBar);

        // Fill based on avg stamina
        const int fillW = static_cast<int>(barW * qBound(0.0, avg, 1.0));
        const QColor fillColor = avg > 0.55 ? QColor("#16a34a") :
                                  avg > 0.3  ? QColor("#d97706") : QColor("#dc2626");
        p.setBrush(fillColor);
        QPainterPath fillBar;
        fillBar.addRoundedRect(left, y, qMax(fillW, barH), barH, 5, 5);
        p.drawPath(fillBar);
    }

    // Category legend at bottom
    if (totalSessions > 0) {
        y += 24;
        QFont legendFont = p.font();
        legendFont.setPointSize(8);
        p.setFont(legendFont);

        const auto counts = categoryCounts();
        int lx = left;
        for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
            const QColor c = colorForCategory(it.key());
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawEllipse(lx, y - 6, 8, 8);

            p.setPen(QColor("#374151"));
            p.drawText(lx + 12, y + 1, it.key() + QStringLiteral(" (") +
                       QString::number(it.value()) + QStringLiteral(")"));

            lx += 80;
            if (lx + 60 > rect.right() - 10)
                break;
        }
    }
}

// ---------- Helpers ----------

void PaperReadingEndurance::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No reading endurance sessions tracked."));
        return;
    }

    infoLabel_->setText(tr("Sessions: %1 | Sustained: %2 | Avg Stamina: %3")
        .arg(entries_.size())
        .arg(sustainedCount())
        .arg(avgStamina(), 0, 'f', 2));
}

void PaperReadingEndurance::loadSettings()
{
    settings_.beginGroup("ReadingEndurance");
    const int size = settings_.beginReadArray("entries");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EnduranceEntry e;
        e.id        = settings_.value("id").toInt();
        e.session   = settings_.value("session").toString();
        e.category  = settings_.value("category").toString();
        e.material  = settings_.value("material").toString();
        e.stamina   = settings_.value("stamina").toReal();
        e.pages     = settings_.value("pages").toInt();
        e.sustained = settings_.value("sustained").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    updateInfo();
    update();
}

void PaperReadingEndurance::saveSettings()
{
    settings_.beginGroup("ReadingEndurance");
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",        e.id);
        settings_.setValue("session",   e.session);
        settings_.setValue("category",  e.category);
        settings_.setValue("material",  e.material);
        settings_.setValue("stamina",   e.stamina);
        settings_.setValue("pages",     e.pages);
        settings_.setValue("sustained", e.sustained);
        settings_.setValue("color",     e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
