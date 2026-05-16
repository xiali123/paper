#include "tools/PaperConfigDiffer.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <algorithm>

// Color palette
static const QColor kBlue   = QColor(QStringLiteral("#3b82f6"));
static const QColor kGreen  = QColor(QStringLiteral("#16a34a"));
static const QColor kAmber  = QColor(QStringLiteral("#d97706"));
static const QColor kRed    = QColor(QStringLiteral("#dc2626"));
static const QColor kPurple = QColor(QStringLiteral("#7c3aed"));

static const QList<QColor> kCategoryColors = { kBlue, kGreen, kAmber, kRed, kPurple };

// ── Constructor ──────────────────────────────────────────────────────────────

PaperConfigDiffer::PaperConfigDiffer(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope,
                QStringLiteral("PaperCrawler"), QStringLiteral("ConfigDiffer"))
{
    setupUI();
    loadSettings();
}

// ── Public API ───────────────────────────────────────────────────────────────

void PaperConfigDiffer::addEntry(const ConfigEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<ConfigEntry> PaperConfigDiffer::entries() const
{
    return entries_;
}

int PaperConfigDiffer::changedCount() const
{
    return static_cast<int>(
        std::count_if(entries_.cbegin(), entries_.cend(),
                      [](const ConfigEntry& e) { return e.changed; }));
}

int PaperConfigDiffer::breakingCount() const
{
    return static_cast<int>(
        std::count_if(entries_.cbegin(), entries_.cend(),
                      [](const ConfigEntry& e) { return e.breaking; }));
}

QMap<QString, int> PaperConfigDiffer::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

// ── Slots ────────────────────────────────────────────────────────────────────

void PaperConfigDiffer::onDiff()
{
    const QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    // Expected format: "key:old->new"
    // The key portion may itself contain a category prefix like "category/key"
    const int colonPos = text.indexOf(QLatin1Char(':'));
    if (colonPos < 0) return;

    const QString key = text.left(colonPos).trimmed();
    const QString rest = text.mid(colonPos + 1).trimmed();

    const int arrowPos = rest.indexOf(QStringLiteral("->"));
    if (arrowPos < 0) return;

    const QString oldVal = rest.left(arrowPos).trimmed();
    const QString newVal = rest.mid(arrowPos + 2).trimmed();

    // Derive category from key — everything before the last '/' or "general"
    const int slashPos = key.lastIndexOf(QLatin1Char('/'));
    const QString category = (slashPos > 0) ? key.left(slashPos) : QStringLiteral("general");

    const bool changed = (oldVal != newVal);

    // Heuristic: consider it breaking if the value type appears to change
    // (numeric -> non-numeric, boolean flip, empty -> non-empty, etc.)
    bool breaking = false;
    if (changed) {
        const bool oldIsNum = !oldVal.isEmpty() && (oldVal.toDouble() != 0.0 || oldVal == QStringLiteral("0"));
        const bool newIsNum = !newVal.isEmpty() && (newVal.toDouble() != 0.0 || newVal == QStringLiteral("0"));
        if (oldIsNum != newIsNum) {
            breaking = true;
        }
        // Boolean-to-non-boolean
        const auto isBool = [](const QString& v) {
            return v == QStringLiteral("true") || v == QStringLiteral("false")
                || v == QStringLiteral("1")   || v == QStringLiteral("0");
        };
        if (isBool(oldVal) != isBool(newVal)) {
            breaking = true;
        }
        // Empty -> non-empty on critical-looking keys
        if (oldVal.isEmpty() && !newVal.isEmpty()) {
            breaking = true;
        }
    }

    // Assign a deterministic color based on category hash
    const int colorIdx = qHash(category) % kCategoryColors.size();
    const QColor color = kCategoryColors[qAbs(colorIdx)];

    const int id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    const int line = id; // Simulated line number

    ConfigEntry entry;
    entry.id       = id;
    entry.key      = key;
    entry.category = category;
    entry.oldValue = oldVal;
    entry.newValue = newVal;
    entry.line     = line;
    entry.changed  = changed;
    entry.breaking = breaking;
    entry.color    = color;

    entries_.append(entry);

    // Persist the selected category filter
    const QString filter = categoryCombo_->currentText();

    // Refresh category combo
    categoryCombo_->clear();
    categoryCombo_->addItem(QStringLiteral("All"));
    const auto cats = categoryCounts().keys();
    for (const auto& c : cats) {
        categoryCombo_->addItem(c);
    }
    // Restore selection if still present
    const int idx = categoryCombo_->findText(filter);
    if (idx >= 0) {
        categoryCombo_->setCurrentIndex(idx);
    }

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();

    emit configDiffed(id, line);
}

void PaperConfigDiffer::onClear()
{
    entries_.clear();
    categoryCombo_->clear();
    categoryCombo_->addItem(QStringLiteral("All"));
    inputField_->clear();
    updateInfo();
    saveSettings();
    update();
}

// ── Painting ─────────────────────────────────────────────────────────────────

void PaperConfigDiffer::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int toolbarH = 48;
    const int topY = toolbarH + 6;

    // Background
    p.fillRect(rect(), QColor(245, 247, 250));

    // Left: diff list (60%), Right: charts + stats (40%)
    const int leftW  = static_cast<int>(w * 0.60);
    const int rightW = w - leftW - 12;
    const int rightX = leftW + 8;
    const int chartH = static_cast<int>((h - topY) * 0.55);

    drawDiffList(p, QRect(0, topY, leftW, h - topY - 4));
    drawCategoryChart(p, QRect(rightX, topY, rightW, chartH));
    drawStats(p, QRect(rightX, topY + chartH + 8, rightW, h - topY - chartH - 12));
}

void PaperConfigDiffer::drawDiffList(QPainter& p, const QRect& rect)
{
    const int x = rect.x() + 4;
    const int y = rect.y();
    const int w = rect.width() - 8;
    const int h = rect.height();

    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 8, 8);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 30, 30));
    p.drawText(x + 10, y + 22, QStringLiteral("Config Differences"));

    QFont baseFont = font();
    p.setFont(baseFont);

    const int rowY0 = y + 34;
    const int rowH  = 52;
    const int visibleRows = (h - 34) / rowH;

    // Determine visible entries based on category filter
    QList<const ConfigEntry*> visible;
    const QString filter = categoryCombo_->currentText();
    for (const auto& e : entries_) {
        if (filter == QStringLiteral("All") || e.category == filter) {
            visible.append(&e);
        }
    }

    // Show last N entries that fit
    int start = 0;
    if (visible.size() > visibleRows) {
        start = visible.size() - visibleRows;
    }

    for (int i = start; i < visible.size(); ++i) {
        const ConfigEntry& e = *visible[i];
        const int ry = rowY0 + (i - start) * rowH;

        if (ry + rowH > y + h) break;

        // Row background — subtle stripe
        if ((i - start) % 2 == 0) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(240, 243, 248));
            p.drawRoundedRect(x + 4, ry, w - 8, rowH - 4, 6, 6);
        }

        // Color indicator bar on left
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(x + 8, ry + 6, 4, rowH - 16, 2, 2);

        // Line number
        QFont monoFont = QFont(QStringLiteral("Monospace"), baseFont.pointSize());
        p.setFont(monoFont);
        p.setPen(QColor(140, 140, 140));
        p.drawText(x + 18, ry + 18, QString::number(e.line));

        // Key
        QFont boldFont = baseFont;
        boldFont.setBold(true);
        p.setFont(boldFont);
        p.setPen(QColor(30, 30, 30));
        p.drawText(x + 50, ry + 18, e.key);

        // Old value (red-ish background)
        const int valY = ry + 26;
        p.setFont(monoFont);

        QRect oldBg(x + 50, valY, w / 2 - 70, 20);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(254, 226, 226));
        p.drawRoundedRect(oldBg, 4, 4);
        p.setPen(kRed);
        const QString oldText = QStringLiteral("- ") + (e.oldValue.isEmpty() ? QStringLiteral("(empty)") : e.oldValue);
        p.drawText(oldBg.adjusted(6, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                   oldText.length() > 40 ? oldText.left(37) + QStringLiteral("...") : oldText);

        // Arrow
        p.setPen(QColor(120, 120, 120));
        p.drawText(x + w / 2 - 12, valY + 14, QStringLiteral("->"));

        // New value (green-ish background)
        QRect newBg(x + w / 2, valY, w / 2 - 60, 20);
        p.setPen(Qt::NoPen);
        p.setBrush(e.changed ? QColor(220, 252, 231) : QColor(243, 244, 246));
        p.drawRoundedRect(newBg, 4, 4);
        p.setPen(e.changed ? kGreen : QColor(120, 120, 120));
        const QString newText = QStringLiteral("+ ") + (e.newValue.isEmpty() ? QStringLiteral("(empty)") : e.newValue);
        p.drawText(newBg.adjusted(6, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                   newText.length() > 40 ? newText.left(37) + QStringLiteral("...") : newText);

        // Breaking marker
        if (e.breaking) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(254, 226, 226));
            const int markerX = x + w - 68;
            p.drawRoundedRect(markerX, ry + 4, 58, 18, 4, 4);
            QFont smallBold = baseFont;
            smallBold.setBold(true);
            smallBold.setPointSize(baseFont.pointSize() - 1);
            p.setFont(smallBold);
            p.setPen(kRed);
            p.drawText(markerX + 2, ry + 17, QStringLiteral("BREAKING"));
        }
    }

    // Empty state
    if (visible.isEmpty()) {
        p.setFont(baseFont);
        p.setPen(QColor(160, 160, 160));
        p.drawText(rect, Qt::AlignCenter,
                   QStringLiteral("No config diffs yet.\nEnter key:old->new and press Diff."));
    }

    // Footer count
    p.setFont(baseFont);
    p.setPen(QColor(120, 120, 120));
    p.drawText(x + 10, y + h - 6,
               QStringLiteral("Showing %1 of %2 entries").arg(visible.size()).arg(entries_.size()));
}

void PaperConfigDiffer::drawCategoryChart(QPainter& p, const QRect& rect)
{
    const int x = rect.x() + 4;
    const int y = rect.y();
    const int w = rect.width() - 8;
    const int h = rect.height();

    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 8, 8);

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 30, 30));
    p.drawText(x + 10, y + 22, QStringLiteral("By Category"));

    const auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont baseFont = font();
        p.setFont(baseFont);
        p.setPen(QColor(160, 160, 160));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   QStringLiteral("No data"));
        return;
    }

    const int maxCount = std::max_element(counts.cbegin(), counts.cend()).value();
    const int barAreaTop = y + 34;
    const int barH = 24;
    const int barGap = 6;
    const int labelW = 90;
    const int barMaxW = w - labelW - 60;

    int idx = 0;
    for (auto it = counts.cbegin(); it != counts.cend() && idx < 8; ++it, ++idx) {
        const int by = barAreaTop + idx * (barH + barGap);
        if (by + barH > y + h - 4) break;

        // Category label
        QFont baseFont = font();
        p.setFont(baseFont);
        p.setPen(QColor(80, 80, 80));
        const QString label = it.key().length() > 12
            ? it.key().left(10) + QStringLiteral("..")
            : it.key();
        p.drawText(x + 8, by + barH - 6, label);

        // Bar
        const int bw = maxCount > 0 ? static_cast<int>(barMaxW * static_cast<double>(it.value()) / maxCount) : 0;
        const QColor& color = kCategoryColors[idx % kCategoryColors.size()];

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(x + labelW, by + 2, std::max(bw, 4), barH - 4, 4, 4);

        // Count badge
        p.setFont(baseFont);
        p.setPen(QColor(60, 60, 60));
        p.drawText(x + labelW + bw + 6, by + barH - 6, QString::number(it.value()));
    }
}

void PaperConfigDiffer::drawStats(QPainter& p, const QRect& rect)
{
    const int x = rect.x() + 4;
    const int y = rect.y();
    const int w = rect.width() - 8;
    const int h = rect.height();

    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 8, 8);

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 30, 30));
    p.drawText(x + 10, y + 22, QStringLiteral("Statistics"));

    const int total    = entries_.size();
    const int changed  = changedCount();
    const int breaking = breakingCount();
    const int unchanged = total - changed;

    struct Stat { QString label; int value; QColor color; };
    const Stat stats[] = {
        { QStringLiteral("Total"),     total,      kBlue   },
        { QStringLiteral("Changed"),   changed,    kAmber  },
        { QStringLiteral("Breaking"),  breaking,   kRed    },
        { QStringLiteral("Unchanged"), unchanged,  kGreen  },
    };

    QFont baseFont = font();
    const int cardW = (w - 40) / 4;
    const int cardH = 56;
    const int cardY = y + 36;

    for (int i = 0; i < 4; ++i) {
        const int cx = x + 8 + i * (cardW + 8);
        const QColor& color = stats[i].color;

        // Card background with tint
        p.setPen(Qt::NoPen);
        p.setBrush(color.lighter(190));
        p.drawRoundedRect(cx, cardY, cardW, cardH, 8, 8);

        // Top accent line
        p.setBrush(color);
        p.drawRoundedRect(cx, cardY, cardW, 4, 8, 8);

        // Value
        QFont bigFont = baseFont;
        bigFont.setBold(true);
        bigFont.setPointSize(bigFont.pointSize() + 4);
        p.setFont(bigFont);
        p.setPen(color);
        p.drawText(QRect(cx, cardY + 6, cardW, 30), Qt::AlignCenter,
                   QString::number(stats[i].value));

        // Label
        p.setFont(baseFont);
        p.setPen(QColor(80, 80, 80));
        p.drawText(QRect(cx, cardY + 34, cardW, 20), Qt::AlignCenter, stats[i].label);
    }

    // Percentage bar
    if (total > 0) {
        const int barY = cardY + cardH + 14;
        const int barH2 = 10;
        const int barW = w - 16;

        // Background track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(230, 230, 230));
        p.drawRoundedRect(x + 8, barY, barW, barH2, 5, 5);

        // Changed segment (amber)
        int offset = 0;
        const int changedW = static_cast<int>(barW * static_cast<double>(changed) / total);
        if (changedW > 0) {
            p.setBrush(kAmber);
            p.drawRoundedRect(x + 8, barY, changedW, barH2, 5, 5);
            offset = changedW;
        }

        // Breaking segment (red) — shown as subset of changed
        const int breakingW = static_cast<int>(barW * static_cast<double>(breaking) / total);
        if (breakingW > 0) {
            p.setBrush(kRed);
            p.drawRoundedRect(x + 8 + offset - breakingW, barY, breakingW, barH2, 5, 5);
        }

        // Unchanged segment (green)
        const int unchangedW = barW - changedW;
        if (unchangedW > 0) {
            p.setBrush(kGreen);
            p.drawRoundedRect(x + 8 + changedW, barY, unchangedW, barH2, 5, 5);
        }

        // Percentage text
        p.setFont(baseFont);
        p.setPen(QColor(100, 100, 100));
        const int pct = static_cast<int>(100.0 * changed / total);
        p.drawText(x + 8, barY + barH2 + 16,
                   QStringLiteral("%1% changed  |  %2% breaking")
                       .arg(pct)
                       .arg(total > 0 ? static_cast<int>(100.0 * breaking / total) : 0));
    }
}

// ── Helpers ──────────────────────────────────────────────────────────────────

void PaperConfigDiffer::updateInfo()
{
    const int total = entries_.size();
    const int changed = changedCount();
    const int breaking = breakingCount();
    infoLabel_->setText(
        QStringLiteral("Entries: %1  |  Changed: %2  |  Breaking: %3")
            .arg(total).arg(changed).arg(breaking));
}

void PaperConfigDiffer::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(8);

    diffBtn_ = new QPushButton(QStringLiteral("Diff"), this);
    diffBtn_->setFixedWidth(64);
    diffBtn_->setStyleSheet(
        QStringLiteral("QPushButton { background: %1; color: white; border-radius: 4px;"
                        " padding: 4px 10px; font-weight: bold; }"
                        "QPushButton:hover { background: %2; }")
            .arg(kBlue.name(), kBlue.darker(120).name()));

    clearBtn_ = new QPushButton(QStringLiteral("Clear"), this);
    clearBtn_->setFixedWidth(64);
    clearBtn_->setStyleSheet(
        QStringLiteral("QPushButton { background: %1; color: white; border-radius: 4px;"
                        " padding: 4px 10px; font-weight: bold; }"
                        "QPushButton:hover { background: %2; }")
            .arg(kRed.name(), kRed.darker(120).name()));

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(QStringLiteral("All"));
    categoryCombo_->setFixedWidth(130);
    categoryCombo_->setStyleSheet(
        QStringLiteral("QComboBox { border: 1px solid #d1d5db; border-radius: 4px;"
                        " padding: 4px 8px; background: white; }"));

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("key:old->new"));
    inputField_->setStyleSheet(
        QStringLiteral("QLineEdit { border: 1px solid #d1d5db; border-radius: 4px;"
                        " padding: 4px 8px; background: white; }"));

    infoLabel_ = new QLabel(QStringLiteral("Entries: 0  |  Changed: 0  |  Breaking: 0"), this);
    infoLabel_->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px;"));

    layout->addWidget(diffBtn_);
    layout->addWidget(clearBtn_);
    layout->addWidget(categoryCombo_);
    layout->addWidget(inputField_, 1);
    layout->addWidget(infoLabel_);

    connect(diffBtn_, &QPushButton::clicked, this, &PaperConfigDiffer::onDiff);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperConfigDiffer::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperConfigDiffer::onDiff);

    setMinimumHeight(320);
}

void PaperConfigDiffer::loadSettings()
{
    const int size = settings_.beginReadArray(QStringLiteral("entries"));
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ConfigEntry e;
        e.id       = settings_.value(QStringLiteral("id")).toInt();
        e.key      = settings_.value(QStringLiteral("key")).toString();
        e.category = settings_.value(QStringLiteral("category")).toString();
        e.oldValue = settings_.value(QStringLiteral("oldValue")).toString();
        e.newValue = settings_.value(QStringLiteral("newValue")).toString();
        e.line     = settings_.value(QStringLiteral("line")).toInt();
        e.changed  = settings_.value(QStringLiteral("changed")).toBool();
        e.breaking = settings_.value(QStringLiteral("breaking")).toBool();
        e.color    = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endArray();

    // Rebuild category combo
    const auto cats = categoryCounts().keys();
    for (const auto& c : cats) {
        categoryCombo_->addItem(c);
    }

    updateInfo();
}

void PaperConfigDiffer::saveSettings()
{
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue(QStringLiteral("id"),       e.id);
        settings_.setValue(QStringLiteral("key"),      e.key);
        settings_.setValue(QStringLiteral("category"), e.category);
        settings_.setValue(QStringLiteral("oldValue"), e.oldValue);
        settings_.setValue(QStringLiteral("newValue"), e.newValue);
        settings_.setValue(QStringLiteral("line"),     e.line);
        settings_.setValue(QStringLiteral("changed"),  e.changed);
        settings_.setValue(QStringLiteral("breaking"), e.breaking);
        settings_.setValue(QStringLiteral("color"),    e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
