#include "workspace/PaperHypothesisBoard.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <QFont>
#include <QPaintEvent>
#include <algorithm>
#include <numeric>

static const QColor kPalette[] = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed"),
};
static constexpr int kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PaperHypothesisBoard::PaperHypothesisBoard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HypothesisBoard")
    , addBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PaperHypothesisBoard::addEntry(const HypothesisEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<HypothesisEntry> PaperHypothesisBoard::entries() const
{
    return entries_;
}

int PaperHypothesisBoard::validatedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.validated)
            ++count;
    }
    return count;
}

qreal PaperHypothesisBoard::avgConfidence() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.confidence;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperHypothesisBoard::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperHypothesisBoard::onAdd()
{
    QString text = inputField_->text().trimmed();
    if (text.isEmpty())
        return;

    static int nextId = 1;

    qreal confidence     = QRandomGenerator::global()->bounded(0.1, 1.0);
    qreal support        = QRandomGenerator::global()->bounded(0.0, confidence);
    qreal contradiction  = QRandomGenerator::global()->bounded(0.0, 1.0 - confidence);

    QString category = categoryCombo_->currentText();
    QColor  color    = kPalette[categoryCombo_->currentIndex() % kPaletteSize];

    HypothesisEntry entry;
    entry.id            = nextId++;
    entry.hypothesis    = text;
    entry.category      = category;
    entry.status        = confidence > 0.8 ? QStringLiteral("Validated") : QStringLiteral("Pending");
    entry.confidence    = confidence;
    entry.support       = support;
    entry.contradiction = contradiction;
    entry.validated     = confidence > 0.8;
    entry.color         = color;

    entries_.append(entry);
    inputField_->clear();

    saveSettings();
    updateInfo();
    update();

    emit hypothesisAdded(entry.id, entry.confidence);
}

void PaperHypothesisBoard::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperHypothesisBoard::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int toolbarH = 48;
    int w = width();
    int h = height() - toolbarH;
    if (h < 0)
        h = 0;
    int y0 = toolbarH;

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    // Split: left = hypothesis list, right = chart + stats
    int leftW   = static_cast<int>(w * 0.55);
    int rightW  = w - leftW;
    int rightH  = h / 2;

    QRect leftRect(0, y0, leftW, h);
    QRect chartRect(leftW, y0, rightW, rightH);
    QRect statsRect(leftW, y0 + rightH, rightW, h - rightH);

    drawHypothesisList(p, leftRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

// ---------------------------------------------------------------------------
// Draw helpers
// ---------------------------------------------------------------------------

void PaperHypothesisBoard::drawHypothesisList(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect.adjusted(6, 6, -6, -6), 10, 10);

    // Title
    QFont titleFont = font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 12, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Hypotheses (%1)").arg(entries_.size()));

    // Entries
    QFont entryFont = font();
    entryFont.setPointSize(10);
    p.setFont(entryFont);

    QFontMetrics fm(entryFont);
    int lineH = 52;
    int startY = rect.top() + 40;
    int maxVisible = (rect.height() - 50) / lineH;

    for (int i = 0; i < std::min(entries_.size(), maxVisible); ++i) {
        const auto& e = entries_.at(i);
        int iy = startY + i * lineH;

        // Color indicator bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left() + 16, iy, 4, lineH - 8, 2, 2);

        // Row background
        QColor rowBg = e.validated ? QColor("#f0fdf4") : QColor("#fefce8");
        p.setBrush(rowBg);
        p.drawRoundedRect(QRect(rect.left() + 26, iy, rect.width() - 44, lineH - 8), 6, 6);

        // Hypothesis text (elided)
        p.setPen(QColor("#334155"));
        QRect textRect(rect.left() + 34, iy + 2, rect.width() - 100, lineH - 12);
        QString elided = fm.elidedText(e.hypothesis, Qt::ElideRight, textRect.width());
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elided);

        // Status badge
        QString badge = e.validated ? QStringLiteral("OK") : QStringLiteral("...");
        QColor badgeColor = e.validated ? QColor("#16a34a") : QColor("#d97706");
        int badgeX = rect.right() - 60;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeColor);
        p.drawRoundedRect(badgeX, iy + 12, 42, 20, 10, 10);
        p.setPen(Qt::white);
        QFont badgeFont = font();
        badgeFont.setPointSize(8);
        badgeFont.setBold(true);
        p.setFont(badgeFont);
        p.drawText(QRect(badgeX, iy + 12, 42, 20), Qt::AlignCenter, badge);

        p.setFont(entryFont);
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont hint = font();
        hint.setPointSize(11);
        p.setFont(hint);
        p.drawText(rect, Qt::AlignCenter, QStringLiteral("No hypotheses yet.\nType one above and click Add."));
    }
}

void PaperHypothesisBoard::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect.adjusted(6, 6, -6, -6), 10, 10);

    // Title
    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 12, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Categories"));

    auto counts = categoryCounts();
    if (counts.isEmpty())
        return;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    int barAreaY = rect.top() + 42;
    int barAreaH = rect.height() - 56;
    if (barAreaH < 20)
        return;

    int barIndex = 0;
    int barWidth = std::max(18, (rect.width() - 48) / counts.size() - 8);
    int maxBarH  = barAreaH - 20;

    QFont catFont = font();
    catFont.setPointSize(8);
    p.setFont(catFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal ratio = static_cast<qreal>(it.value()) / static_cast<qreal>(total);
        int bh = static_cast<int>(ratio * maxBarH);
        if (bh < 4)
            bh = 4;

        int bx = rect.left() + 20 + barIndex * (barWidth + 10);
        int by = barAreaY + maxBarH - bh;

        QColor c = kPalette[barIndex % kPaletteSize];

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(bx, by, barWidth, bh, 4, 4);

        // Label
        p.setPen(QColor("#475569"));
        QFontMetrics fm(catFont);
        QString label = fm.elidedText(it.key(), Qt::ElideRight, barWidth + 6);
        p.drawText(QRect(bx - 3, barAreaY + maxBarH + 2, barWidth + 6, 14),
                   Qt::AlignCenter, label);

        // Count on top of bar
        p.setPen(QColor("#1e293b"));
        QFont numFont = font();
        numFont.setPointSize(8);
        numFont.setBold(true);
        p.setFont(numFont);
        p.drawText(QRect(bx, by - 14, barWidth, 14), Qt::AlignCenter,
                   QString::number(it.value()));
        p.setFont(catFont);

        ++barIndex;
    }
}

void PaperHypothesisBoard::drawStats(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect.adjusted(6, 6, -6, -6), 10, 10);

    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 12, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Statistics"));

    QFont statFont = font();
    statFont.setPointSize(10);
    p.setFont(statFont);

    int total      = entries_.size();
    int validated  = validatedCount();
    qreal avgConf  = avgConfidence();
    int pending    = total - validated;

    int sx = rect.left() + 20;
    int sy = rect.top() + 40;
    int lineH = 22;

    // Stat cards
    struct Stat { QString label; QString value; QColor color; };
    Stat stats[] = {
        { QStringLiteral("Total"),       QString::number(total),                       QColor("#3b82f6") },
        { QStringLiteral("Validated"),   QString::number(validated),                   QColor("#16a34a") },
        { QStringLiteral("Pending"),     QString::number(pending),                     QColor("#d97706") },
        { QStringLiteral("Avg Conf"),    QString::number(avgConf, 'f', 2),             QColor("#7c3aed") },
    };

    for (int i = 0; i < 4; ++i) {
        int cy = sy + i * lineH;

        // Color dot
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color);
        p.drawEllipse(sx, cy + 4, 8, 8);

        // Label
        p.setPen(QColor("#64748b"));
        p.drawText(sx + 16, cy + 2, 80, 18, Qt::AlignLeft | Qt::AlignVCenter,
                   stats[i].label);

        // Value
        QFont valFont = statFont;
        valFont.setBold(true);
        p.setFont(valFont);
        p.setPen(QColor("#1e293b"));
        p.drawText(sx + 100, cy + 2, 80, 18, Qt::AlignLeft | Qt::AlignVCenter,
                   stats[i].value);
        p.setFont(statFont);
    }

    // Confidence bar
    int barY = sy + 4 * lineH + 8;
    int barW = rect.width() - 48;
    if (barW > 0) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(sx, barY, barW, 10, 5, 5);

        int fillW = static_cast<int>(avgConf * barW);
        if (fillW > 0) {
            // Gradient color based on confidence
            QColor barColor = avgConf > 0.8 ? QColor("#16a34a") :
                              avgConf > 0.5 ? QColor("#d97706") : QColor("#dc2626");
            p.setBrush(barColor);
            p.drawRoundedRect(sx, barY, fillW, 10, 5, 5);
        }
    }
}

// ---------------------------------------------------------------------------
// UI helpers
// ---------------------------------------------------------------------------

void PaperHypothesisBoard::setupUI()
{
    auto* toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 8, 8, 4);
    toolbar->setSpacing(6);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("Enter hypothesis..."));
    inputField_->setMinimumWidth(200);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(QStringLiteral("Theory"));
    categoryCombo_->addItem(QStringLiteral("Method"));
    categoryCombo_->addItem(QStringLiteral("Data"));
    categoryCombo_->addItem(QStringLiteral("Result"));
    categoryCombo_->addItem(QStringLiteral("Other"));
    categoryCombo_->setFixedWidth(110);

    addBtn_ = new QPushButton(QStringLiteral("Add"), this);
    addBtn_->setFixedWidth(64);

    clearBtn_ = new QPushButton(QStringLiteral("Clear"), this);
    clearBtn_->setFixedWidth(64);

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    toolbar->addWidget(inputField_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    toolbar->addWidget(infoLabel_);

    // Main layout -- toolbar on top, the rest is custom-painted
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(0);
    main->addLayout(toolbar);
    main->addStretch(1);

    setMinimumSize(560, 380);

    connect(addBtn_, &QPushButton::clicked, this, &PaperHypothesisBoard::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHypothesisBoard::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperHypothesisBoard::onAdd);
}

void PaperHypothesisBoard::updateInfo()
{
    int total     = entries_.size();
    int validated = validatedCount();
    qreal avg     = avgConfidence();

    infoLabel_->setText(QStringLiteral("Total: %1  |  Validated: %2  |  Avg confidence: %3")
                            .arg(total)
                            .arg(validated)
                            .arg(avg, 0, 'f', 2));
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PaperHypothesisBoard::loadSettings()
{
    int size = settings_.beginReadArray(QStringLiteral("entries"));
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HypothesisEntry e;
        e.id            = settings_.value(QStringLiteral("id")).toInt();
        e.hypothesis    = settings_.value(QStringLiteral("hypothesis")).toString();
        e.category      = settings_.value(QStringLiteral("category")).toString();
        e.status        = settings_.value(QStringLiteral("status")).toString();
        e.confidence    = settings_.value(QStringLiteral("confidence")).toReal();
        e.support       = settings_.value(QStringLiteral("support")).toReal();
        e.contradiction = settings_.value(QStringLiteral("contradiction")).toReal();
        e.validated     = settings_.value(QStringLiteral("validated")).toBool();
        e.color         = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
    update();
}

void PaperHypothesisBoard::saveSettings()
{
    settings_.beginWriteArray(QStringLiteral("entries"), entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"),            e.id);
        settings_.setValue(QStringLiteral("hypothesis"),    e.hypothesis);
        settings_.setValue(QStringLiteral("category"),      e.category);
        settings_.setValue(QStringLiteral("status"),        e.status);
        settings_.setValue(QStringLiteral("confidence"),    e.confidence);
        settings_.setValue(QStringLiteral("support"),       e.support);
        settings_.setValue(QStringLiteral("contradiction"), e.contradiction);
        settings_.setValue(QStringLiteral("validated"),     e.validated);
        settings_.setValue(QStringLiteral("color"),         e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
