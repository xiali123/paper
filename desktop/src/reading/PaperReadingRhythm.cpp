#include "reading/PaperReadingRhythm.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QMessageBox>
#include <algorithm>
#include <cmath>

namespace {
static const QColor kPalette[] = {
    QColor("#3b82f6"),
    QColor("#16a34a"),
    QColor("#d97706"),
    QColor("#dc2626"),
    QColor("#7c3aed"),
};
static constexpr int kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);

static const QString kPatterns[] = {
    QStringLiteral("steady"),
    QStringLiteral("burst"),
    QStringLiteral("declining"),
    QStringLiteral("rising"),
    QStringLiteral("fluctuating"),
};
static constexpr int kPatternCount = sizeof(kPatterns) / sizeof(kPatterns[0]);
}

PaperReadingRhythm::PaperReadingRhythm(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingRhythm")
    , measureBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingRhythm::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Input row
    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Enter reading period (e.g. Morning, Evening, 2026-05-12)"));
    inputField_->setMinimumWidth(280);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("Deep Reading"));
    categoryCombo_->addItem(tr("Skimming"));
    categoryCombo_->addItem(tr("Review"));
    categoryCombo_->addItem(tr("Note-taking"));
    categoryCombo_->addItem(tr("Reference Lookup"));
    categoryCombo_->setMinimumWidth(140);

    measureBtn_ = new QPushButton(tr("Measure"), this);
    measureBtn_->setFixedWidth(100);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setFixedWidth(80);

    inputRow->addWidget(inputField_);
    inputRow->addWidget(categoryCombo_);
    inputRow->addWidget(measureBtn_);
    inputRow->addWidget(clearBtn_);
    inputRow->addStretch();

    mainLayout->addLayout(inputRow);

    // Info label
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 13px;");
    updateInfo();
    mainLayout->addWidget(infoLabel_);

    // Canvas area with generous minimum size
    setMinimumSize(700, 480);

    // Connections
    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingRhythm::onMeasure);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingRhythm::onClear);

    // Enter key triggers measurement
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperReadingRhythm::onMeasure);
}

void PaperReadingRhythm::addEntry(const RhythmEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    update();
    saveSettings();
}

QList<RhythmEntry> PaperReadingRhythm::entries() const
{
    return entries_;
}

int PaperReadingRhythm::peakCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.peak) ++count;
    }
    return count;
}

qreal PaperReadingRhythm::avgIntensity() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.intensity;
    }
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingRhythm::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperReadingRhythm::onMeasure()
{
    QString period = inputField_->text().trimmed();
    if (period.isEmpty()) {
        period = QDateTime::currentDateTime().toString(Qt::ISODate);
    }

    QString category = categoryCombo_->currentText();
    auto* rng = QRandomGenerator::global();

    qreal intensity  = rng->bounded(100) / 100.0;           // [0.00, 0.99]
    qreal consistency = 0.3 + (rng->bounded(70) / 100.0);   // [0.30, 0.99]
    qreal focus      = 0.2 + (rng->bounded(80) / 100.0);   // [0.20, 0.99]
    bool peak        = intensity > 0.8;

    int patternIdx   = rng->bounded(kPatternCount);
    int colorIdx     = rng->bounded(kPaletteSize);

    static int nextId = 1;
    RhythmEntry entry;
    entry.id         = nextId++;
    entry.period     = period;
    entry.category   = category;
    entry.pattern    = kPatterns[patternIdx];
    entry.intensity  = intensity;
    entry.consistency = consistency;
    entry.focus      = focus;
    entry.peak       = peak;
    entry.color      = kPalette[colorIdx];

    addEntry(entry);
    emit rhythmMeasured(entry.id, entry.intensity);

    inputField_->clear();
}

void PaperReadingRhythm::onClear()
{
    entries_.clear();
    updateInfo();
    update();
    saveSettings();
}

// ── Painting ──────────────────────────────────────────────────────────────

void PaperReadingRhythm::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    int w = width();
    int margin = 20;

    // Layout zones
    QRect rhythmRect(margin, 130, w - 2 * margin, 200);
    QRect categoryRect(margin, 350, w / 2 - margin, 140);
    QRect statsRect(w / 2 + margin / 2, 350, w / 2 - margin - margin / 2, 140);

    drawRhythmView(p, rhythmRect);
    drawCategoryChart(p, categoryRect);
    drawStats(p, statsRect);
}

void PaperReadingRhythm::drawRhythmView(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(16, 12, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Reading Rhythm"));

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont hintFont = p.font();
        hintFont.setPointSize(11);
        hintFont.setBold(false);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter, tr("No measurements yet. Click \"Measure\" to begin."));
        return;
    }

    // Draw intensity bars
    QFont baseFont;
    baseFont.setPointSize(9);
    baseFont.setBold(false);
    p.setFont(baseFont);

    const int barAreaTop = rect.top() + 40;
    const int barAreaHeight = rect.height() - 56;
    const int barAreaLeft = rect.left() + 16;
    const int barAreaWidth = rect.width() - 32;
    const int maxVisible = qMin(entries_.size(), 30);
    const int startIdx = qMax(0, entries_.size() - maxVisible);
    const qreal barWidth = static_cast<qreal>(barAreaWidth) / maxVisible - 4.0;

    // Horizontal grid lines
    p.setPen(QPen(QColor("#e2e8f0"), 1, Qt::DashLine));
    for (int i = 0; i <= 4; ++i) {
        int y = barAreaTop + static_cast<int>(barAreaHeight * (1.0 - i / 4.0));
        p.drawLine(barAreaLeft, y, barAreaLeft + barAreaWidth, y);
    }

    // Peak threshold line at 0.8
    {
        int y = barAreaTop + static_cast<int>(barAreaHeight * 0.2);
        p.setPen(QPen(QColor("#dc2626"), 1, Qt::DashDotLine));
        p.drawLine(barAreaLeft, y, barAreaLeft + barAreaWidth, y);
        p.setPen(QColor("#dc2626"));
        QFont smallFont;
        smallFont.setPointSize(8);
        p.setFont(smallFont);
        p.drawText(barAreaLeft + barAreaWidth + 4, y + 4, tr("Peak"));
    }

    // Bars
    for (int i = 0; i < maxVisible; ++i) {
        const RhythmEntry& entry = entries_[startIdx + i];
        qreal x = barAreaLeft + i * (barWidth + 4.0);
        int barH = static_cast<int>(barAreaHeight * entry.intensity);
        int barY = barAreaTop + barAreaHeight - barH;

        // Bar body
        QColor barColor = entry.color;
        if (entry.peak) {
            barColor = barColor.lighter(115);
        }
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(QRectF(x, barY, barWidth, barH), 3.0, 3.0);

        // Peak indicator dot
        if (entry.peak) {
            p.setBrush(QColor("#dc2626"));
            p.drawEllipse(QPointF(x + barWidth / 2.0, barY - 6), 3.0, 3.0);
        }
    }

    // X-axis labels for last few entries
    p.setPen(QColor("#64748b"));
    QFont labelFont;
    labelFont.setPointSize(8);
    p.setFont(labelFont);
    const int labelStep = qMax(1, maxVisible / 6);
    for (int i = 0; i < maxVisible; i += labelStep) {
        const RhythmEntry& entry = entries_[startIdx + i];
        qreal x = barAreaLeft + i * (barWidth + 4.0);
        QString shortPeriod = entry.period.length() > 8
            ? entry.period.left(8) + QStringLiteral("..")
            : entry.period;
        p.drawText(QRectF(x - 4, barAreaTop + barAreaHeight + 2, barWidth + 16, 14),
                   Qt::AlignLeft | Qt::AlignTop, shortPeriod);
    }
}

void PaperReadingRhythm::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(14, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("By Category"));

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont hintFont;
        hintFont.setPointSize(10);
        hintFont.setBold(false);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter, tr("No data"));
        return;
    }

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        total += it.value();
    }

    const int donutCenterX = rect.left() + 60;
    const int donutCenterY = rect.top() + 80;
    const int outerRadius = 40;
    const int innerRadius = 22;

    int colorIdx = 0;
    qreal startAngle = 0.0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal span = 360.0 * it.value() / total;
        QColor segColor = kPalette[colorIdx % kPaletteSize];

        p.setPen(Qt::NoPen);
        p.setBrush(segColor);
        p.drawPie(QRect(donutCenterX - outerRadius, donutCenterY - outerRadius,
                        outerRadius * 2, outerRadius * 2),
                  static_cast<int>(startAngle * 16),
                  static_cast<int>(span * 16));
        startAngle += span;
        ++colorIdx;
    }

    // Center hole
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(QPoint(donutCenterX, donutCenterY), innerRadius, innerRadius);

    // Center label
    p.setPen(QColor("#334155"));
    QFont centerFont;
    centerFont.setPointSize(10);
    centerFont.setBold(true);
    p.setFont(centerFont);
    p.drawText(QRect(donutCenterX - innerRadius, donutCenterY - 8, innerRadius * 2, 16),
               Qt::AlignCenter, QString::number(total));

    // Legend
    QFont legendFont;
    legendFont.setPointSize(9);
    legendFont.setBold(false);
    p.setFont(legendFont);

    int legendY = rect.top() + 38;
    int legendX = rect.left() + 120;
    colorIdx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QColor segColor = kPalette[colorIdx % kPaletteSize];
        p.setPen(Qt::NoPen);
        p.setBrush(segColor);
        p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);

        p.setPen(QColor("#475569"));
        p.drawText(QRect(legendX + 14, legendY - 2, 140, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1 (%2)").arg(it.key()).arg(it.value()));
        legendY += 18;
        ++colorIdx;
    }
}

void PaperReadingRhythm::drawStats(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(14, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Statistics"));

    QFont statFont;
    statFont.setPointSize(11);
    statFont.setBold(false);
    p.setFont(statFont);
    p.setPen(QColor("#475569"));

    const int leftMargin = rect.left() + 16;
    int y = rect.top() + 38;
    const int lineH = 22;

    p.drawText(leftMargin, y, tr("Total entries: %1").arg(entries_.size()));
    y += lineH;

    p.drawText(leftMargin, y, tr("Peak readings: %1").arg(peakCount()));
    y += lineH;

    p.drawText(leftMargin, y, tr("Avg intensity: %1").arg(QString::number(avgIntensity(), 'f', 2)));
    y += lineH;

    // Average focus
    qreal avgFocus = 0.0;
    if (!entries_.isEmpty()) {
        for (const auto& e : entries_) avgFocus += e.focus;
        avgFocus /= entries_.size();
    }
    p.drawText(leftMargin, y, tr("Avg focus: %1").arg(QString::number(avgFocus, 'f', 2)));
    y += lineH;

    // Most common pattern
    if (!entries_.isEmpty()) {
        QMap<QString, int> patternCounts;
        for (const auto& e : entries_) patternCounts[e.pattern]++;
        QString topPattern;
        int topCount = 0;
        for (auto it = patternCounts.constBegin(); it != patternCounts.constEnd(); ++it) {
            if (it.value() > topCount) {
                topCount = it.value();
                topPattern = it.key();
            }
        }
        p.drawText(leftMargin, y, tr("Dominant pattern: %1").arg(topPattern));
    }
}

void PaperReadingRhythm::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No rhythm measurements recorded. Enter a reading period and click Measure."));
        return;
    }

    qreal avg = avgIntensity();
    int peaks = peakCount();
    auto counts = categoryCounts();

    QString topCategory;
    int topCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > topCount) {
            topCount = it.value();
            topCategory = it.key();
        }
    }

    infoLabel_->setText(tr("%1 entries | Avg intensity: %2 | Peaks: %3 | Top category: %4")
                            .arg(entries_.size())
                            .arg(QString::number(avg, 'f', 2))
                            .arg(peaks)
                            .arg(topCategory));
}

// ── Persistence ───────────────────────────────────────────────────────────

void PaperReadingRhythm::loadSettings()
{
    entries_.clear();
    int count = settings_.beginReadArray("entries");
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        RhythmEntry entry;
        entry.id          = settings_.value("id").toInt();
        entry.period      = settings_.value("period").toString();
        entry.category    = settings_.value("category").toString();
        entry.pattern     = settings_.value("pattern").toString();
        entry.intensity   = settings_.value("intensity").toReal();
        entry.consistency = settings_.value("consistency").toReal();
        entry.focus       = settings_.value("focus").toReal();
        entry.peak        = settings_.value("peak").toBool();
        entry.color       = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingRhythm::saveSettings()
{
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        const RhythmEntry& entry = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id",          entry.id);
        settings_.setValue("period",      entry.period);
        settings_.setValue("category",    entry.category);
        settings_.setValue("pattern",     entry.pattern);
        settings_.setValue("intensity",   entry.intensity);
        settings_.setValue("consistency", entry.consistency);
        settings_.setValue("focus",       entry.focus);
        settings_.setValue("peak",        entry.peak);
        settings_.setValue("color",       entry.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
