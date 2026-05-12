#include "tools/PaperFeatureFlag.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QFontMetrics>
#include <algorithm>

namespace {
static const QColor kPalette[] = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};
static constexpr int kPaletteSize = 5;
}

PaperFeatureFlag::PaperFeatureFlag(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FeatureFlag")
{
    setupUI();
    loadSettings();
}

void PaperFeatureFlag::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Toolbar row
    auto* toolbar = new QHBoxLayout();

    toggleBtn_ = new QPushButton("Toggle");
    toggleBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(toggleBtn_, &QPushButton::clicked, this, &PaperFeatureFlag::onToggle);
    toolbar->addWidget(toggleBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Frontend", "Backend", "Infrastructure", "Experiment", "Security"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFeatureFlag::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    // Input field
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter feature flag name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    // Info label
    infoLabel_ = new QLabel("Manage feature flags and rollout percentages");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}

// ---------- public helpers ----------

void PaperFeatureFlag::addEntry(const FlagEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<FlagEntry> PaperFeatureFlag::entries() const { return entries_; }

int PaperFeatureFlag::enabledCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.enabled) ++c;
    return c;
}

qreal PaperFeatureFlag::avgRollout() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.rollout;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperFeatureFlag::categoryCounts() const {
    QMap<QString, int> map;
    for (const auto& e : entries_) map[e.category]++;
    return map;
}

// ---------- slots ----------

void PaperFeatureFlag::onToggle() {
    QString flagName = inputField_->text().trimmed();
    if (flagName.isEmpty()) {
        infoLabel_->setText("Please enter a flag name");
        return;
    }

    FlagEntry entry;
    entry.id = QRandomGenerator::global()->bounded(1, 999999);
    entry.name = flagName;
    entry.category = categoryCombo_->currentText() == "All"
        ? QStringList{"Frontend", "Backend", "Infrastructure", "Experiment", "Security"}
            .at(QRandomGenerator::global()->bounded(5))
        : categoryCombo_->currentText();

    const QStringList envs = {"production", "staging", "development", "canary"};
    entry.environment = envs.at(QRandomGenerator::global()->bounded(envs.size()));

    entry.enabled = QRandomGenerator::global()->bounded(2) == 1;
    entry.rollout = static_cast<qreal>(QRandomGenerator::global()->bounded(101)) / 100.0;
    entry.description = QStringLiteral("Feature flag: %1 (%2 rollout)")
        .arg(entry.name).arg(QString::number(entry.rollout * 100, 'f', 0) + "%");
    entry.stable = entry.rollout >= 1.0;
    entry.color = kPalette[QRandomGenerator::global()->bounded(kPaletteSize)];

    addEntry(entry);
    inputField_->clear();
    emit flagToggled(entry.id, entry.rollout);
}

void PaperFeatureFlag::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
    infoLabel_->setText("All feature flags cleared");
}

// ---------- painting ----------

void PaperFeatureFlag::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int contentTop = 130;

    // Flag list (left half)
    drawFlagList(p, QRect(10, contentTop, w / 2 - 15, height() - contentTop - 10));

    // Category chart (top-right)
    drawCategoryChart(p, QRect(w / 2 + 5, contentTop, w / 2 - 15, (height() - contentTop) / 2 - 5));

    // Stats (bottom-right)
    drawStats(p, QRect(w / 2 + 5, contentTop + (height() - contentTop) / 2 + 5,
                       w / 2 - 15, (height() - contentTop) / 2 - 10));
}

void PaperFeatureFlag::drawFlagList(QPainter& p, const QRect& rect) {
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor(148, 163, 184));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Feature Flags");

    QFont normalFont = font();
    normalFont.setPointSize(normalFont.pointSize() - 1);
    p.setFont(normalFont);

    if (entries_.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.adjusted(10, 35, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   "No flags yet. Enter a name and click Toggle.");
        return;
    }

    const int rowHeight = 56;
    int y = rect.top() + 35;
    int visibleCount = 0;
    const int maxVisible = (rect.height() - 40) / rowHeight;

    for (int i = entries_.size() - 1; i >= 0 && visibleCount < maxVisible; --i, ++visibleCount) {
        const auto& entry = entries_[i];
        QRect rowRect(rect.left() + 8, y, rect.width() - 16, rowHeight - 4);

        // Row background
        p.setPen(Qt::NoPen);
        p.setBrush(entry.enabled ? QColor(236, 253, 245) : QColor(254, 242, 242));
        p.drawRoundedRect(rowRect, 6, 6);

        // Color indicator dot
        p.setBrush(entry.color);
        p.drawEllipse(rowRect.left() + 10, rowRect.top() + 10, 10, 10);

        // Name
        QFont boldFont = normalFont;
        boldFont.setBold(true);
        p.setFont(boldFont);
        p.setPen(QColor(30, 41, 59));
        p.drawText(rowRect.adjusted(28, 4, -60, -rowHeight / 2), Qt::AlignLeft | Qt::AlignVCenter,
                   entry.name);

        // Status badge
        QFont smallFont = normalFont;
        smallFont.setPointSize(smallFont.pointSize() - 1);
        p.setFont(smallFont);
        QString status = entry.enabled ? (entry.stable ? "STABLE" : "ON") : "OFF";
        QColor badgeColor = entry.enabled ? (entry.stable ? QColor("#16a34a") : QColor("#3b82f6"))
                                          : QColor("#dc2626");
        QRect badgeRect(rowRect.right() - 55, rowRect.top() + 5, 48, 18);
        p.setPen(Qt::NoPen);
        p.setBrush(badgeColor);
        p.drawRoundedRect(badgeRect, 9, 9);
        p.setPen(Qt::white);
        p.setFont(smallFont);
        p.drawText(badgeRect, Qt::AlignCenter, status);

        // Category + env + rollout
        p.setFont(smallFont);
        p.setPen(QColor(100, 116, 139));
        QString meta = QStringLiteral("%1 | %2 | Rollout: %3%4")
            .arg(entry.category, entry.environment,
                 QString::number(entry.rollout * 100, 'f', 0),
                 entry.stable ? " (stable)" : "");
        p.drawText(rowRect.adjusted(28, rowHeight / 2 - 6, -10, 0),
                   Qt::AlignLeft | Qt::AlignTop, meta);

        // Rollout progress bar
        int barY = rowRect.bottom() - 10;
        int barW = rowRect.width() - 40;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(QRect(rowRect.left() + 28, barY, barW, 4), 2, 2);
        p.setBrush(badgeColor);
        p.drawRoundedRect(QRect(rowRect.left() + 28, barY,
                                static_cast<int>(barW * entry.rollout), 4), 2, 2);

        y += rowHeight;
    }
}

void PaperFeatureFlag::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);
    p.setPen(QColor(148, 163, 184));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Category Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont normalFont = font();
        p.setFont(normalFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.adjusted(10, 35, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   "No data to display");
        return;
    }

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) total += it.value();

    QFont normalFont = font();
    normalFont.setPointSize(normalFont.pointSize() - 1);
    p.setFont(normalFont);

    // Donut chart
    int chartSize = qMin(rect.width(), rect.height()) - 60;
    int cx = rect.left() + rect.width() / 2 - 40;
    int cy = rect.top() + 40 + chartSize / 2;
    int outerR = chartSize / 2;
    int innerR = outerR * 55 / 100;

    qreal startAngle = 0.0;
    int colorIdx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal sweep = 360.0 * it.value() / static_cast<qreal>(total);
        QColor sliceColor = kPalette[colorIdx % kPaletteSize];

        p.setPen(Qt::NoPen);
        p.setBrush(sliceColor);

        QPainterPath path;
        path.moveTo(cx, cy);
        path.arcTo(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                   static_cast<qreal>(16 * startAngle),
                   static_cast<qreal>(16 * sweep));
        path.closeSubpath();

        QPainterPath hole;
        hole.addEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);
        path -= hole;
        p.drawPath(path);

        startAngle += sweep;
        ++colorIdx;
    }

    // Center label
    QFont centerFont = font();
    centerFont.setBold(true);
    centerFont.setPointSize(centerFont.pointSize() + 2);
    p.setFont(centerFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(QRect(cx - 25, cy - 12, 50, 24), Qt::AlignCenter, QString::number(total));

    // Legend on the right of chart
    int legendX = cx + outerR + 20;
    int legendY = rect.top() + 40;
    p.setFont(normalFont);
    colorIdx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QColor c = kPalette[colorIdx % kPaletteSize];
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);
        p.setPen(QColor(51, 65, 85));
        p.drawText(legendX + 16, legendY + 10,
                   QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()));
        legendY += 20;
        ++colorIdx;
    }
}

void PaperFeatureFlag::drawStats(QPainter& p, const QRect& rect) {
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);
    p.setPen(QColor(148, 163, 184));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statFont = font();
    statFont.setPointSize(statFont.pointSize());
    p.setFont(statFont);

    int y = rect.top() + 35;
    int spacing = 28;
    int statCount = entries_.size();
    int enabled = enabledCount();
    int disabled = statCount - enabled;
    qreal avgRoll = avgRollout();

    // Total
    p.setPen(QColor(100, 116, 139));
    p.drawText(rect.adjusted(14, y, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Total Flags: %1").arg(statCount));
    y += spacing;

    // Enabled / Disabled as bars
    p.drawText(rect.adjusted(14, y, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Enabled: %1   Disabled: %2").arg(enabled).arg(disabled));
    y += spacing;

    // Enabled ratio bar
    int barX = rect.left() + 14;
    int barW = rect.width() - 28;
    int barH = 12;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(226, 232, 240));
    p.drawRoundedRect(QRect(barX, y, barW, barH), 6, 6);
    if (statCount > 0) {
        qreal ratio = static_cast<qreal>(enabled) / static_cast<qreal>(statCount);
        p.setBrush(QColor("#16a34a"));
        p.drawRoundedRect(QRect(barX, y, qMax(static_cast<int>(barW * ratio), barH), barH), 6, 6);
    }
    y += spacing;

    // Average rollout
    p.setPen(QColor(100, 116, 139));
    p.drawText(rect.adjusted(14, y, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Avg Rollout: %1%")
                   .arg(QString::number(avgRoll * 100, 'f', 1)));
    y += spacing;

    // Stable count
    int stableCount = 0;
    for (const auto& e : entries_)
        if (e.stable) ++stableCount;
    p.drawText(rect.adjusted(14, y, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Stable Flags: %1").arg(stableCount));
}

// ---------- info / persistence ----------

void PaperFeatureFlag::updateInfo() {
    int total = entries_.size();
    int enabled = enabledCount();
    qreal avg = avgRollout();

    infoLabel_->setText(
        QStringLiteral("Flags: %1 | Enabled: %2 | Avg Rollout: %3%")
            .arg(total)
            .arg(enabled)
            .arg(QString::number(avg * 100, 'f', 1)));
}

void PaperFeatureFlag::loadSettings() {
    int size = settings_.beginReadArray("flags");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FlagEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.name = settings_.value("name").toString();
        entry.category = settings_.value("category").toString();
        entry.environment = settings_.value("environment").toString();
        entry.enabled = settings_.value("enabled").toBool();
        entry.rollout = settings_.value("rollout").toReal();
        entry.description = settings_.value("description").toString();
        entry.stable = settings_.value("stable").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFeatureFlag::saveSettings() {
    settings_.beginWriteArray("flags");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("name", e.name);
        settings_.setValue("category", e.category);
        settings_.setValue("environment", e.environment);
        settings_.setValue("enabled", e.enabled);
        settings_.setValue("rollout", e.rollout);
        settings_.setValue("description", e.description);
        settings_.setValue("stable", e.stable);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
