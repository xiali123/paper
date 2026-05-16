#include "reading/PaperReadingZone.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QFontMetrics>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace {

QColor categoryColor(const QString& category)
{
    static const QHash<QString, QColor> colors = {
        {"Focus", QColor("#3b82f6")},
        {"Browse", QColor("#16a34a")},
        {"Skim", QColor("#7c3aed")},
        {"Study", QColor("#d97706")},
    };
    return colors.value(category, QColor("#6b7280"));
}

qreal normalizeAngle(qreal angle)
{
    while (angle < 0) angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;
    return angle;
}

void drawStatRow(QPainter& p, int x, int y, int w, int h,
                 const QString& label, const QString& value,
                 const QColor& accent)
{
    p.setPen(QColor("#64748b"));
    p.setFont(QFont("Sans", 9));
    p.drawText(QRect(x, y, w * 2 / 3, h), Qt::AlignLeft | Qt::AlignVCenter, label);

    p.setPen(Qt::NoPen);
    p.setBrush(accent);
    p.drawEllipse(QPointF(x + w * 2 / 3 - 8, y + h / 2.0), 3, 3);

    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 9, QFont::Bold));
    p.drawText(QRect(x + w * 2 / 3, y, w / 3, h), Qt::AlignRight | Qt::AlignVCenter, value);
}

} // anonymous namespace

PaperReadingZone::PaperReadingZone(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperReadingZone")
{
    setupUI();
    loadSettings();
}

void PaperReadingZone::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Top control bar
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(8);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem("All");
    categoryCombo_->addItem("Focus");
    categoryCombo_->addItem("Browse");
    categoryCombo_->addItem("Skim");
    categoryCombo_->addItem("Study");
    topBar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter zone...");
    topBar->addWidget(inputField_);

    trackBtn_ = new QPushButton("Track", this);
    topBar->addWidget(trackBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    topBar->addWidget(clearBtn_);

    mainLayout->addLayout(topBar);

    // Bottom info label
    infoLabel_ = new QLabel("Entries: 0 | Deep Work: 0% | Avg Focus: 0.00", this);
    infoLabel_->setWordWrap(true);
    mainLayout->addWidget(infoLabel_);

    // Expand paint area
    mainLayout->addStretch(1);

    setMinimumHeight(420);

    connect(trackBtn_, &QPushButton::clicked, this, &PaperReadingZone::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingZone::onClear);
}

// ── paintEvent ──────────────────────────────────────────────────────

void PaperReadingZone::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int topHalf = h / 2;
    int bottomHalf = h - topHalf;
    int leftW = w / 2;
    int rightW = w - leftW;

    drawZoneView(p, QRect(0, 0, w, topHalf));
    drawCategoryChart(p, QRect(0, topHalf, leftW, bottomHalf));
    drawStats(p, QRect(leftW, topHalf, rightW, bottomHalf));
}

// ── drawZoneView ────────────────────────────────────────────────────

void PaperReadingZone::drawZoneView(QPainter& p, const QRect& rect)
{
    // Background
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.strokePath(bgPath, QPen(QColor("#e2e8f0"), 1));

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 11));
        p.drawText(rect, Qt::AlignCenter, "No zones tracked yet");
        return;
    }

    QString filterCat = categoryCombo_->currentText();
    QList<ZoneEntry> visible;
    for (const auto& e : entries_) {
        if (filterCat == "All" || e.category == filterCat)
            visible.append(e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 11));
        p.drawText(rect, Qt::AlignCenter, "No entries for this category");
        return;
    }

    int cols = std::max(1, std::min(static_cast<int>(visible.size()), 4));
    int rows = static_cast<int>(std::ceil(static_cast<qreal>(visible.size()) / cols));
    int pad = 8;
    int gap = 6;
    int availW = rect.width() - 2 * pad - (cols - 1) * gap;
    int availH = rect.height() - 2 * pad - (rows - 1) * gap;
    int cellW = availW / cols;
    int cellH = availH / rows;

    QFontMetrics fm(QFont("Sans", 9));
    QFontMetrics fmTitle(QFont("Sans", 10, QFont::Bold));

    for (int i = 0; i < visible.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int x = rect.x() + pad + col * (cellW + gap);
        int y = rect.y() + pad + row * (cellH + gap);
        QRect cell(x, y, cellW, cellH);

        const ZoneEntry& entry = visible[i];

        // Card background
        QPainterPath cardPath;
        cardPath.addRoundedRect(cell, 6, 6);
        QColor baseColor = entry.color;
        baseColor.setAlpha(25);
        p.fillPath(cardPath, baseColor);
        p.strokePath(cardPath, QPen(entry.color, 1.5));

        // Focus ring (drawn in upper-right area of card)
        int ringRadius = std::min(cellW, cellH) / 4;
        int ringCX = cell.x() + cellW - ringRadius - 8;
        int ringCY = cell.y() + ringRadius + 8;
        QRectF ringRect(ringCX - ringRadius, ringCY - ringRadius, ringRadius * 2, ringRadius * 2);

        // Background ring
        p.setPen(QPen(QColor("#e2e8f0"), 3));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(ringRect);

        // Focus arc
        qreal focusAngle = entry.focus * 360.0;
        p.setPen(QPen(entry.color, 3));
        p.drawArc(ringRect, static_cast<int>(90 * 16),
                  static_cast<int>(-normalizeAngle(focusAngle) * 16));

        // Focus percentage text inside ring
        p.setPen(entry.color);
        p.setFont(QFont("Sans", 7, QFont::Bold));
        QString focusText = QString::number(static_cast<int>(entry.focus * 100)) + "%";
        QRectF focusTextRect = ringRect;
        p.drawText(focusTextRect, Qt::AlignCenter, focusText);

        // Zone name
        p.setPen(QColor("#1e293b"));
        p.setFont(QFont("Sans", 10, QFont::Bold));
        int textLeft = cell.x() + 8;
        int textWidth = ringCX - ringRadius - textLeft - 4;
        if (textWidth < 20) textWidth = cellW - 16;
        QRect nameRect(textLeft, cell.y() + 6, textWidth, fmTitle.height());
        QString elidedName = fmTitle.elidedText(entry.zone, Qt::ElideRight, textWidth);
        p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, elidedName);

        // Category label
        p.setPen(QColor("#64748b"));
        p.setFont(QFont("Sans", 8));
        p.drawText(QRect(textLeft, nameRect.bottom(), textWidth, fm.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, entry.category);

        // Minutes
        p.setPen(QColor("#334155"));
        p.setFont(QFont("Sans", 9));
        QString minText = QString::number(entry.minutes) + " min";
        p.drawText(QRect(textLeft, cell.bottom() - fm.height() - 14,
                         cellW / 2, fm.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, minText);

        // Deep work indicator
        if (entry.deepWork) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#3b82f6"));
            int dotR = 4;
            int dotX = cell.x() + cellW / 2 + 12;
            int dotY = cell.bottom() - fm.height() / 2 - 12;
            p.drawEllipse(QPointF(dotX, dotY), dotR, dotR);

            p.setPen(QColor("#3b82f6"));
            p.setFont(QFont("Sans", 7));
            p.drawText(QRect(dotX + 6, dotY - fm.height() / 2, 60, fm.height()),
                       Qt::AlignLeft | Qt::AlignVCenter, "Deep");
        }
    }
}

// ── drawCategoryChart ───────────────────────────────────────────────

void PaperReadingZone::drawCategoryChart(QPainter& p, const QRect& rect)
{
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.strokePath(bgPath, QPen(QColor("#e2e8f0"), 1));

    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(QRect(rect.x() + 8, rect.y() + 6, rect.width() - 16, 20),
               Qt::AlignLeft, "Categories");

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignCenter, "No data");
        return;
    }

    QMap<QString, int> counts = categoryCounts();
    QStringList categories = {"Focus", "Browse", "Skim", "Study"};
    int total = 0;
    for (const auto& cat : categories)
        total += counts.value(cat, 0);

    if (total == 0) return;

    // Donut chart dimensions
    int chartSize = std::min(rect.width(), rect.height()) - 80;
    chartSize = std::max(chartSize, 60);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + (rect.height() - 50) / 2;
    int outerR = chartSize / 2;
    int innerR = outerR * 55 / 100;

    qreal startAngle = 90.0;

    for (const auto& cat : categories) {
        int count = counts.value(cat, 0);
        if (count == 0) continue;

        qreal sweep = (static_cast<qreal>(count) / total) * 360.0;
        QColor color = categoryColor(cat);

        QPainterPath slice;
        QRectF outerRect(cx - outerR, cy - outerR, outerR * 2, outerR * 2);
        QRectF innerRect(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        slice.arcMoveTo(outerRect, startAngle);
        slice.arcTo(outerRect, startAngle, sweep);
        slice.arcTo(innerRect, startAngle + sweep, -sweep);
        slice.closeSubpath();

        p.fillPath(slice, color);

        // Label on the slice
        qreal midAngle = normalizeAngle(startAngle + sweep / 2.0);
        qreal labelR = (outerR + innerR) / 2.0;
        qreal rad = qDegreesToRadians(midAngle);
        int lx = cx + static_cast<int>(labelR * std::cos(rad));
        int ly = cy - static_cast<int>(labelR * std::sin(rad));

        p.setPen(Qt::white);
        p.setFont(QFont("Sans", 8, QFont::Bold));
        p.drawText(QRect(lx - 20, ly - 8, 40, 16), Qt::AlignCenter,
                   QString::number(count));

        startAngle += sweep;
    }

    // Center total
    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 14, QFont::Bold));
    p.drawText(QRect(cx - 20, cy - 12, 40, 24), Qt::AlignCenter,
               QString::number(total));
    p.setFont(QFont("Sans", 7));
    p.drawText(QRect(cx - 20, cy + 10, 40, 14), Qt::AlignCenter, "total");

    // Legend below chart
    int legendY = cy + outerR + 10;
    int legendX = rect.x() + 8;
    p.setFont(QFont("Sans", 8));
    for (const auto& cat : categories) {
        if (counts.value(cat, 0) == 0) continue;
        p.setPen(Qt::NoPen);
        p.setBrush(categoryColor(cat));
        p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);
        p.setPen(QColor("#334155"));
        p.drawText(legendX + 14, legendY + 9, cat);
        QFontMetrics fm8(p.font());
        legendX += fm8.horizontalAdvance(cat) + 28;
        if (legendX > rect.right() - 40) {
            legendX = rect.x() + 8;
            legendY += 16;
        }
    }
}

// ── drawStats ───────────────────────────────────────────────────────

void PaperReadingZone::drawStats(QPainter& p, const QRect& rect)
{
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.strokePath(bgPath, QPen(QColor("#e2e8f0"), 1));

    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(QRect(rect.x() + 8, rect.y() + 6, rect.width() - 16, 20),
               Qt::AlignLeft, "Statistics");

    int cy = rect.y() + 36;
    int lineH = 28;
    int leftPad = rect.x() + 12;

    // Total entries
    drawStatRow(p, leftPad, cy, rect.width() - 24, lineH,
                "Total Entries", QString::number(entries_.size()),
                QColor("#3b82f6"));
    cy += lineH;

    // Total minutes
    int totalMin = 0;
    for (const auto& e : entries_)
        totalMin += e.minutes;
    drawStatRow(p, leftPad, cy, rect.width() - 24, lineH,
                "Total Minutes", QString::number(totalMin),
                QColor("#16a34a"));
    cy += lineH;

    // Average focus
    drawStatRow(p, leftPad, cy, rect.width() - 24, lineH,
                "Avg Focus", QString::number(avgFocus(), 'f', 2),
                QColor("#7c3aed"));
    cy += lineH;

    // Deep work count
    drawStatRow(p, leftPad, cy, rect.width() - 24, lineH,
                "Deep Work", QString::number(deepWorkCount()),
                QColor("#d97706"));
    cy += lineH;

    // Deep work percentage
    qreal deepPct = entries_.isEmpty() ? 0.0
                    : (static_cast<qreal>(deepWorkCount()) / entries_.size()) * 100.0;
    drawStatRow(p, leftPad, cy, rect.width() - 24, lineH,
                "Deep Work %", QString::number(deepPct, 'f', 1) + "%",
                QColor("#dc2626"));
    cy += lineH + 4;

    // Focus bar
    if (!entries_.isEmpty()) {
        qreal avg = avgFocus();
        int barW = rect.width() - 40;
        int barH = 12;
        int barX = rect.x() + 20;
        int barY = cy;

        // Background bar
        QPainterPath barBg;
        barBg.addRoundedRect(QRectF(barX, barY, barW, barH), barH / 2.0, barH / 2.0);
        p.fillPath(barBg, QColor("#e2e8f0"));

        // Fill bar
        int fillW = static_cast<int>(barW * avg);
        if (fillW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRectF(barX, barY, fillW, barH), barH / 2.0, barH / 2.0);
            QColor barColor = avg > 0.7 ? QColor("#16a34a") :
                              avg > 0.4 ? QColor("#d97706") : QColor("#dc2626");
            p.fillPath(barFill, barColor);
        }

        p.setPen(QColor("#64748b"));
        p.setFont(QFont("Sans", 7));
        p.drawText(QRect(barX, barY + barH + 2, barW, 12), Qt::AlignCenter,
                   "Focus Level");
    }
}

// ── slots ───────────────────────────────────────────────────────────

void PaperReadingZone::onTrack()
{
    QString zone = inputField_->text().trimmed();
    if (zone.isEmpty()) return;

    ZoneEntry entry;
    entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.zone = zone;

    QString cat = categoryCombo_->currentText();
    if (cat == "All") cat = "Focus"; // default when "All" is selected
    entry.category = cat;

    entry.activity = cat.toLower();
    entry.focus = QRandomGenerator::global()->generateDouble();
    entry.minutes = QRandomGenerator::global()->bounded(5, 121);
    entry.deepWork = entry.focus > 0.7;
    entry.color = categoryColor(entry.category);

    entries_.append(entry);
    inputField_->clear();

    updateInfo();
    saveSettings();
    update();
    emit zoneEntered(entry.id, entry.focus);
}

void PaperReadingZone::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ── info / settings ────────────────────────────────────────────────

void PaperReadingZone::updateInfo()
{
    int count = entries_.size();
    int deep = deepWorkCount();
    qreal deepPct = (count > 0) ? (static_cast<qreal>(deep) / count) * 100.0 : 0.0;
    qreal focus = avgFocus();

    infoLabel_->setText(
        QString("Entries: %1 | Deep Work: %2% | Avg Focus: %3")
            .arg(count)
            .arg(QString::number(deepPct, 'f', 0))
            .arg(QString::number(focus, 'f', 2))
    );
}

void PaperReadingZone::loadSettings()
{
    int size = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ZoneEntry e;
        e.id = settings_.value("id").toInt();
        e.zone = settings_.value("zone").toString();
        e.category = settings_.value("category").toString();
        e.activity = settings_.value("activity").toString();
        e.focus = settings_.value("focus").toReal();
        e.minutes = settings_.value("minutes").toInt();
        e.deepWork = settings_.value("deepWork").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingZone::saveSettings()
{
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const ZoneEntry& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("zone", e.zone);
        settings_.setValue("category", e.category);
        settings_.setValue("activity", e.activity);
        settings_.setValue("focus", e.focus);
        settings_.setValue("minutes", e.minutes);
        settings_.setValue("deepWork", e.deepWork);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}

// ── public helpers ──────────────────────────────────────────────────

QList<ZoneEntry> PaperReadingZone::entries() const
{
    return entries_;
}

int PaperReadingZone::deepWorkCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.deepWork) ++count;
    }
    return count;
}

qreal PaperReadingZone::avgFocus() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.focus;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingZone::categoryCounts() const
{
    QMap<QString, int> counts;
    counts["Focus"] = 0;
    counts["Browse"] = 0;
    counts["Skim"] = 0;
    counts["Study"] = 0;
    for (const auto& e : entries_) {
        if (counts.contains(e.category))
            counts[e.category]++;
        else
            counts[e.category] = 1;
    }
    return counts;
}
