#include "reading/PaperReadingPace.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <QFont>
#include <QPaintEvent>
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
} // namespace

PaperReadingPace::PaperReadingPace(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingPace")
    , measureBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingPace::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    measureBtn_ = new QPushButton("Measure", this);
    clearBtn_ = new QPushButton("Clear", this);
    categoryCombo_ = new QComboBox(this);
    inputField_ = new QLineEdit(this);
    infoLabel_ = new QLabel(this);

    categoryCombo_->addItems({"All", "Survey", "Theory", "Empirical", "Review", "Case Study"});
    inputField_->setPlaceholderText("Enter paper name...");
    infoLabel_->setWordWrap(true);
    infoLabel_->setMinimumWidth(180);

    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingPace::onMeasure);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingPace::onClear);

    auto* controlLayout = new QVBoxLayout();
    controlLayout->setSpacing(6);
    controlLayout->addWidget(inputField_);
    controlLayout->addWidget(categoryCombo_);
    controlLayout->addWidget(measureBtn_);
    controlLayout->addWidget(clearBtn_);
    controlLayout->addWidget(infoLabel_);
    controlLayout->addStretch();

    mainLayout->addLayout(controlLayout, 1);

    // The remaining horizontal space is the custom-paint canvas area.
    // paintEvent covers the region to the right of the controls.
    setMinimumSize(720, 480);
}

void PaperReadingPace::addEntry(const PaceEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<PaceEntry> PaperReadingPace::entries() const
{
    return entries_;
}

int PaperReadingPace::onTrackCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.onTrack)
            ++count;
    }
    return count;
}

qreal PaperReadingPace::avgSpeed() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.speed;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingPace::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingPace::onMeasure()
{
    QString paper = inputField_->text().trimmed();
    if (paper.isEmpty())
        return;

    QString category = categoryCombo_->currentText();

    QRandomGenerator* rng = QRandomGenerator::global();
    qreal speed = rng->bounded(0.1, 1.0);
    qreal consistency = rng->bounded(0.2, 1.0);
    qreal retention = rng->bounded(0.3, 1.0);

    // Derive a human-readable pace label.
    QString pace;
    if (speed >= 0.8)
        pace = "Fast";
    else if (speed >= 0.5)
        pace = "Moderate";
    else
        pace = "Slow";

    bool onTrack = speed > 0.5;

    static int nextId = 1;
    PaceEntry entry;
    entry.id = nextId++;
    entry.paper = paper;
    entry.category = category;
    entry.pace = pace;
    entry.speed = speed;
    entry.consistency = consistency;
    entry.retention = retention;
    entry.onTrack = onTrack;
    entry.color = kPalette[entry.id % kPaletteSize];

    entries_.append(entry);
    inputField_->clear();

    emit paceMeasured(entry.id, entry.speed);

    updateInfo();
    saveSettings();
    update();
}

void PaperReadingPace::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingPace::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int controlWidth = 220;
    int margin = 12;

    QRect paceRect(margin + controlWidth, margin,
                   width() - controlWidth - 2 * margin,
                   height() - 2 * margin);
    drawPaceView(p, paceRect);

    int halfW = paceRect.width() / 2;
    QRect catRect(paceRect.left(), paceRect.top(), halfW, paceRect.height());
    QRect statsRect(paceRect.left() + halfW, paceRect.top(), halfW, paceRect.height());

    drawCategoryChart(p, catRect);
    drawStats(p, statsRect);
}

void PaperReadingPace::drawPaceView(QPainter& p, const QRect& rect)
{
    p.save();

    // Background panel.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(245, 247, 250));
    p.drawRoundedRect(rect, 12, 12);

    QFont titleFont("Segoe UI", 11, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 12, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Reading Pace");

    if (entries_.isEmpty()) {
        QFont hintFont("Segoe UI", 9);
        p.setFont(hintFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect, Qt::AlignCenter, "No measurements yet.\nEnter a paper name and press Measure.");
        p.restore();
        return;
    }

    // Draw speed bars for each entry.
    int barAreaTop = rect.top() + 40;
    int barAreaHeight = rect.height() - 56;
    int barHeight = std::max(14, std::min(28, barAreaHeight / static_cast<int>(entries_.size()) - 4));
    int gap = 4;
    int labelWidth = 120;
    int barMaxWidth = rect.width() - labelWidth - 80;

    QFont entryFont("Segoe UI", 8);
    p.setFont(entryFont);

    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        const auto& entry = entries_[i];
        int y = barAreaTop + i * (barHeight + gap);
        if (y + barHeight > rect.bottom() - 12)
            break;

        // Truncated paper name.
        QFontMetrics fm(entryFont);
        QString name = fm.elidedText(entry.paper, Qt::ElideRight, labelWidth - 8);
        p.setPen(QColor("#334155"));
        p.drawText(QRect(rect.left() + 16, y, labelWidth, barHeight), Qt::AlignVCenter | Qt::AlignLeft, name);

        // Speed bar.
        int barWidth = static_cast<int>(entry.speed * barMaxWidth);
        QRect barRect(rect.left() + 16 + labelWidth, y + 2, barWidth, barHeight - 4);
        p.setPen(Qt::NoPen);
        p.setBrush(entry.color);
        p.drawRoundedRect(barRect, 4, 4);

        // Speed value text.
        p.setPen(QColor("#475569"));
        p.drawText(QRect(barRect.right() + 8, y, 50, barHeight),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(entry.speed, 'f', 2));
    }

    p.restore();
}

void PaperReadingPace::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.save();

    QFont titleFont("Segoe UI", 10, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 12, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.restore();
        return;
    }

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxCount = std::max(maxCount, it.value());
    maxCount = std::max(maxCount, 1);

    int chartTop = rect.top() + 38;
    int chartHeight = rect.height() - 52;
    int barThickness = 16;
    int spacing = 6;
    int leftMargin = 16;
    int labelW = 90;
    int barMax = rect.width() - labelW - leftMargin - 40;

    QFont catFont("Segoe UI", 8);
    p.setFont(catFont);
    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y = chartTop + idx * (barThickness + spacing);
        if (y + barThickness > rect.bottom() - 8)
            break;

        // Label.
        p.setPen(QColor("#334155"));
        p.drawText(QRect(rect.left() + leftMargin, y, labelW, barThickness),
                   Qt::AlignVCenter | Qt::AlignLeft, it.key());

        // Bar.
        int w = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * barMax);
        QColor barColor = kPalette[idx % kPaletteSize];
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.left() + leftMargin + labelW, y + 2, w, barThickness - 4, 3, 3);

        // Count text.
        p.setPen(QColor("#475569"));
        p.drawText(QRect(rect.left() + leftMargin + labelW + w + 6, y, 30, barThickness),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(it.value()));

        ++idx;
    }

    p.restore();
}

void PaperReadingPace::drawStats(QPainter& p, const QRect& rect)
{
    p.save();

    QFont titleFont("Segoe UI", 10, QFont::Bold);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 12, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statsFont("Segoe UI", 9);
    p.setFont(statsFont);

    qreal avgSpd = avgSpeed();
    int onTrack = onTrackCount();
    int total = static_cast<int>(entries_.size());
    qreal onTrackPct = total > 0 ? (static_cast<qreal>(onTrack) / total) * 100.0 : 0.0;

    // Compute average consistency and retention.
    qreal avgCons = 0.0;
    qreal avgRet = 0.0;
    if (total > 0) {
        for (const auto& e : entries_) {
            avgCons += e.consistency;
            avgRet += e.retention;
        }
        avgCons /= total;
        avgRet /= total;
    }

    int left = rect.left() + 16;
    int y = rect.top() + 40;
    int lineH = 22;

    auto drawStatLine = [&](const QString& label, const QString& value, const QColor& color) {
        p.setPen(QColor("#64748b"));
        p.drawText(left, y, label);
        p.setPen(color);
        p.drawText(left + 130, y, value);
        y += lineH;
    };

    drawStatLine("Total papers:", QString::number(total), QColor("#1e293b"));
    drawStatLine("On track:", QString("%1 (%2%)").arg(onTrack).arg(onTrackPct, 0, 'f', 1), kPalette[1]);
    drawStatLine("Avg speed:", QString::number(avgSpd, 'f', 3), kPalette[0]);
    drawStatLine("Avg consistency:", QString::number(avgCons, 'f', 3), kPalette[2]);
    drawStatLine("Avg retention:", QString::number(avgRet, 'f', 3), kPalette[4]);

    // Draw on-track progress arc.
    if (total > 0) {
        int arcSize = 80;
        int arcX = left + 40;
        int arcY = y + 12;
        int penWidth = 8;

        // Background arc.
        p.setPen(QPen(QColor("#e2e8f0"), penWidth, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(arcX, arcY, arcSize, arcSize, 0, 360 * 16);

        // Filled arc.
        int spanAngle = static_cast<int>(onTrackPct / 100.0 * 360 * 16);
        p.setPen(QPen(kPalette[1], penWidth, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(arcX, arcY, arcSize, arcSize, 90 * 16, -spanAngle);

        // Center text.
        QFont pctFont("Segoe UI", 11, QFont::Bold);
        p.setFont(pctFont);
        p.setPen(QColor("#1e293b"));
        QString pctText = QString::number(onTrackPct, 'f', 0) + "%";
        QRect arcRect(arcX, arcY, arcSize, arcSize);
        p.drawText(arcRect, Qt::AlignCenter, pctText);
    }

    p.restore();
}

void PaperReadingPace::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText("No entries.");
        return;
    }

    qreal avgSpd = avgSpeed();
    int onTrack = onTrackCount();
    int total = static_cast<int>(entries_.size());

    infoLabel_->setText(
        QString("Papers: %1\nOn track: %2\nAvg speed: %3")
            .arg(total)
            .arg(onTrack)
            .arg(avgSpd, 0, 'f', 2));
}

void PaperReadingPace::loadSettings()
{
    int size = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PaceEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.paper = settings_.value("paper").toString();
        entry.category = settings_.value("category").toString();
        entry.pace = settings_.value("pace").toString();
        entry.speed = settings_.value("speed").toReal();
        entry.consistency = settings_.value("consistency").toReal();
        entry.retention = settings_.value("retention").toReal();
        entry.onTrack = settings_.value("onTrack").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();

    updateInfo();
    update();
}

void PaperReadingPace::saveSettings()
{
    settings_.beginWriteArray("entries");
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("paper", e.paper);
        settings_.setValue("category", e.category);
        settings_.setValue("pace", e.pace);
        settings_.setValue("speed", e.speed);
        settings_.setValue("consistency", e.consistency);
        settings_.setValue("retention", e.retention);
        settings_.setValue("onTrack", e.onTrack);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
