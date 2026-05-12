#include "reading/PaperReadingBacklog.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QtMath>

namespace {

const QColor kPalette[] = {
    QColor("#3b82f6"),
    QColor("#16a34a"),
    QColor("#d97706"),
    QColor("#dc2626"),
    QColor("#7c3aed"),
};

const QStringList kCategories = {
    "Machine Learning",
    "Computer Vision",
    "NLP",
    "Systems",
    "Theory",
    "Other",
};

const QStringList kPriorities = {
    "High",
    "Medium",
    "Low",
};

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PaperReadingBacklog::PaperReadingBacklog(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingBacklog")
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

void PaperReadingBacklog::addEntry(const BacklogEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<BacklogEntry> PaperReadingBacklog::entries() const
{
    return entries_;
}

int PaperReadingBacklog::urgentCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.urgent)
            ++count;
    }
    return count;
}

qreal PaperReadingBacklog::avgDifficulty() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.difficulty;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingBacklog::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperReadingBacklog::onAdd()
{
    const QString paper = inputField_->text().trimmed();
    if (paper.isEmpty())
        return;

    const QString category = categoryCombo_->currentText();
    const int    pages     = QRandomGenerator::global()->bounded(5, 61);
    const qreal  diff      = QRandomGenerator::global()->bounded(10, 101) / 10.0;
    const QString priority = kPriorities.at(
        QRandomGenerator::global()->bounded(kPriorities.size()));

    BacklogEntry entry;
    entry.id         = QDateTime::currentMSecsSinceEpoch() % 100000;
    entry.paper      = paper;
    entry.category   = category;
    entry.priority   = priority;
    entry.pages      = pages;
    entry.difficulty = diff;
    entry.addedDate  = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    entry.urgent     = (priority == "High");
    entry.color      = kPalette[QRandomGenerator::global()->bounded(
                          static_cast<int>(std::size(kPalette)))];

    entries_.append(entry);
    emit backlogAdded(entry.id, entry.difficulty);

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingBacklog::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperReadingBacklog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int toolbarH = 50;
    const int margin   = 12;

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    // Left panel -- backlog list
    const QRect listRect(margin, toolbarH + margin,
                         w * 2 / 3 - margin * 2, h - toolbarH - margin * 2);
    drawBacklogList(p, listRect);

    // Right column
    const int rightX = w * 2 / 3 + margin;
    const int rightW = w / 3 - margin * 2;
    const int chartH = (h - toolbarH - margin * 3) / 2;

    const QRect chartRect(rightX, toolbarH + margin, rightW, chartH);
    drawCategoryChart(p, chartRect);

    const QRect statsRect(rightX, toolbarH + margin * 2 + chartH, rightW, chartH);
    drawStats(p, statsRect);
}

// ---------------------------------------------------------------------------
// drawBacklogList
// ---------------------------------------------------------------------------

void PaperReadingBacklog::drawBacklogList(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Drop shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 15));
    p.drawRoundedRect(rect.adjusted(2, 2, 2, 2), 12, 12);

    // Re-draw card on top of shadow
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 12, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               QString("Reading Backlog (%1)").arg(entries_.size()));

    // Separator
    const int sepY = rect.top() + 40;
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.drawLine(rect.left() + 16, sepY, rect.right() - 16, sepY);

    // Column header
    QFont headerFont = font();
    headerFont.setBold(true);
    headerFont.setPointSize(headerFont.pointSize() - 1);
    p.setFont(headerFont);
    p.setPen(QColor("#94a3b8"));

    const int colY = sepY + 8;
    const int lh   = 36;
    p.drawText(rect.left() + 20,  colY, 200, lh, Qt::AlignVCenter | Qt::AlignLeft, "Paper");
    p.drawText(rect.left() + 230, colY, 120, lh, Qt::AlignVCenter | Qt::AlignLeft, "Category");
    p.drawText(rect.left() + 360, colY, 70,  lh, Qt::AlignVCenter | Qt::AlignLeft, "Pages");
    p.drawText(rect.left() + 440, colY, 80,  lh, Qt::AlignVCenter | Qt::AlignLeft, "Difficulty");
    p.drawText(rect.left() + 530, colY, 80,  lh, Qt::AlignVCenter | Qt::AlignLeft, "Priority");
    p.drawText(rect.left() + 620, colY, 100, lh, Qt::AlignVCenter | Qt::AlignLeft, "Date");

    // Entries
    QFont entryFont = font();
    entryFont.setPointSize(entryFont.pointSize() - 1);
    p.setFont(entryFont);

    const int startY = colY + lh;
    const int maxY   = rect.bottom() - 8;
    int y = startY;

    for (int i = 0; i < entries_.size() && y + lh < maxY; ++i) {
        const auto& e = entries_[i];

        // Row background for urgent
        if (e.urgent) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38, 18));
            p.drawRoundedRect(QRect(rect.left() + 8, y, rect.width() - 16, lh - 2), 6, 6);
        }

        // Color dot
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(QPoint(rect.left() + 14, y + lh / 2), 5, 5);

        // Text columns
        p.setPen(e.urgent ? QColor("#dc2626") : QColor("#334155"));
        p.drawText(rect.left() + 24,  y, 200, lh, Qt::AlignVCenter | Qt::AlignLeft,
                   e.paper.length() > 24 ? e.paper.left(22) + ".." : e.paper);

        p.setPen(QColor("#475569"));
        p.drawText(rect.left() + 230, y, 120, lh, Qt::AlignVCenter | Qt::AlignLeft, e.category);

        p.setPen(QColor("#64748b"));
        p.drawText(rect.left() + 360, y, 70,  lh, Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(e.pages));

        // Difficulty bar
        {
            const int barX = rect.left() + 440;
            const int barW = 60;
            const int barH = 8;
            const int barY = y + lh / 2 - barH / 2;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#e2e8f0"));
            p.drawRoundedRect(barX, barY, barW, barH, 4, 4);
            const qreal fill = qBound(0.0, e.difficulty / 10.0, 1.0);
            QColor barColor = fill < 0.4 ? QColor("#16a34a") :
                              fill < 0.7 ? QColor("#d97706") : QColor("#dc2626");
            p.setBrush(barColor);
            p.drawRoundedRect(barX, barY, static_cast<int>(barW * fill), barH, 4, 4);
        }

        // Priority badge
        {
            const int badgeX = rect.left() + 530;
            QColor badgeBg;
            QColor badgeFg;
            if (e.priority == "High") {
                badgeBg = QColor(220, 38, 38, 30);
                badgeFg = QColor("#dc2626");
            } else if (e.priority == "Medium") {
                badgeBg = QColor(217, 119, 6, 30);
                badgeFg = QColor("#d97706");
            } else {
                badgeBg = QColor(22, 163, 74, 30);
                badgeFg = QColor("#16a34a");
            }
            p.setPen(Qt::NoPen);
            p.setBrush(badgeBg);
            p.drawRoundedRect(badgeX, y + 6, 60, lh - 14, 10, 10);
            p.setPen(badgeFg);
            p.setFont(entryFont);
            p.drawText(badgeX, y + 6, 60, lh - 14, Qt::AlignCenter, e.priority);
        }

        // Date
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.left() + 620, y, 100, lh, Qt::AlignVCenter | Qt::AlignLeft, e.addedDate);

        y += lh;
    }

    // Empty state
    if (entries_.isEmpty()) {
        QFont emptyFont = font();
        emptyFont.setPointSize(emptyFont.pointSize() + 1);
        p.setFont(emptyFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect, Qt::AlignCenter, "No papers in backlog.\nAdd one above to get started.");
    }
}

// ---------------------------------------------------------------------------
// drawCategoryChart
// ---------------------------------------------------------------------------

void PaperReadingBacklog::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 15));
    p.drawRoundedRect(rect.adjusted(2, 2, 2, 2), 12, 12);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 10, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               "Category Distribution");

    if (entries_.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPointSize(hintFont.pointSize() - 1);
        p.setFont(hintFont);
        p.setPen(QColor("#cbd5e1"));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    const QMap<QString, int> counts = categoryCounts();
    int maxCount = 0;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        maxCount = qMax(maxCount, it.value());
    if (maxCount == 0)
        maxCount = 1;

    const int chartLeft   = rect.left() + 20;
    const int chartRight  = rect.right() - 20;
    const int chartTop    = rect.top() + 42;
    const int chartBottom = rect.bottom() - 24;
    const int chartW      = chartRight - chartLeft;
    const int chartH      = chartBottom - chartTop;

    // Grid lines
    p.setPen(QPen(QColor("#f1f5f9"), 1));
    for (int i = 0; i <= 4; ++i) {
        const int gy = chartBottom - (chartH * i / 4);
        p.drawLine(chartLeft, gy, chartRight, gy);
    }

    // Bars
    const int barCount  = counts.size();
    const int barGap    = 12;
    const int barW      = qMax(20, (chartW - barGap * (barCount + 1)) / barCount);
    int x = chartLeft + (chartW - barCount * (barW + barGap) + barGap) / 2;

    QFont labelFont = font();
    labelFont.setPointSize(labelFont.pointSize() - 2);
    p.setFont(labelFont);

    int colorIdx = 0;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        const int bh = static_cast<int>(
            static_cast<qreal>(chartH) * it.value() / maxCount);
        const QRect barRect(x, chartBottom - bh, barW, bh);

        // Bar with rounded top
        p.setPen(Qt::NoPen);
        QColor barColor = kPalette[colorIdx % std::size(kPalette)];
        p.setBrush(barColor);
        p.drawRoundedRect(barRect, 6, 6);

        // Value on top
        p.setPen(QColor("#475569"));
        p.drawText(QRect(x - 4, chartBottom - bh - 20, barW + 8, 18),
                   Qt::AlignCenter, QString::number(it.value()));

        // Category label below
        p.setPen(QColor("#64748b"));
        p.drawText(QRect(x - 8, chartBottom + 4, barW + 16, 18),
                   Qt::AlignCenter, it.key());

        x += barW + barGap;
        ++colorIdx;
    }
}

// ---------------------------------------------------------------------------
// drawStats
// ---------------------------------------------------------------------------

void PaperReadingBacklog::drawStats(QPainter& p, const QRect& rect)
{
    // Card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 15));
    p.drawRoundedRect(rect.adjusted(2, 2, 2, 2), 12, 12);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 12, 12);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(16, 10, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               "Statistics");

    const int total  = entries_.size();
    const int urgent = urgentCount();
    const qreal avgD = avgDifficulty();
    int totalPages   = 0;
    for (const auto& e : entries_)
        totalPages += e.pages;

    // Stat cards in a 2x2 grid
    struct Stat {
        QString label;
        QString value;
        QColor  color;
    };

    const Stat stats[] = {
        {"Total Papers",   QString::number(total),                  QColor("#3b82f6")},
        {"Urgent",         QString::number(urgent),                 QColor("#dc2626")},
        {"Avg Difficulty", QString::number(avgD, 'f', 1),           QColor("#d97706")},
        {"Total Pages",    QString::number(totalPages),             QColor("#16a34a")},
    };

    const int gridTop  = rect.top() + 44;
    const int gridLeft = rect.left() + 16;
    const int cellW    = (rect.width() - 48) / 2;
    const int cellH    = (rect.height() - 44 - 32) / 2;

    QFont valueFont = font();
    valueFont.setBold(true);
    valueFont.setPointSize(valueFont.pointSize() + 4);

    QFont labelFont = font();
    labelFont.setPointSize(labelFont.pointSize() - 1);

    for (int i = 0; i < 4; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        const int cx  = gridLeft + col * (cellW + 16);
        const int cy  = gridTop  + row * (cellH + 8);
        const QRect cellRect(cx, cy, cellW, cellH);

        // Mini card background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(240));
        p.drawRoundedRect(cellRect, 10, 10);

        // Left accent border
        p.setBrush(stats[i].color);
        p.drawRoundedRect(QRect(cx, cy, 5, cellH), 2, 2);

        // Value
        p.setFont(valueFont);
        p.setPen(stats[i].color);
        p.drawText(cellRect.adjusted(16, 4, -8, -20), Qt::AlignLeft | Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setFont(labelFont);
        p.setPen(QColor("#64748b"));
        p.drawText(cellRect.adjusted(16, 0, -8, -6), Qt::AlignLeft | Qt::AlignBottom,
                   stats[i].label);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void PaperReadingBacklog::updateInfo()
{
    const int total  = entries_.size();
    const int urgent = urgentCount();
    const qreal avg  = avgDifficulty();

    infoLabel_->setText(
        QString("Backlog: %1 papers | Urgent: %2 | Avg difficulty: %3")
            .arg(total)
            .arg(urgent)
            .arg(avg, 0, 'f', 1));
}

void PaperReadingBacklog::loadSettings()
{
    const int size = settings_.beginReadArray("entries");
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BacklogEntry e;
        e.id         = settings_.value("id").toInt();
        e.paper      = settings_.value("paper").toString();
        e.category   = settings_.value("category").toString();
        e.priority   = settings_.value("priority").toString();
        e.pages      = settings_.value("pages").toInt();
        e.difficulty = settings_.value("difficulty").toDouble();
        e.addedDate  = settings_.value("addedDate").toString();
        e.urgent     = settings_.value("urgent").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
    update();
}

void PaperReadingBacklog::saveSettings()
{
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id",         e.id);
        settings_.setValue("paper",      e.paper);
        settings_.setValue("category",   e.category);
        settings_.setValue("priority",   e.priority);
        settings_.setValue("pages",      e.pages);
        settings_.setValue("difficulty", e.difficulty);
        settings_.setValue("addedDate",  e.addedDate);
        settings_.setValue("urgent",     e.urgent);
        settings_.setValue("color",      e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}

// ---------------------------------------------------------------------------
// setupUI
// ---------------------------------------------------------------------------

void PaperReadingBacklog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar row
    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(10);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter paper name...");
    inputField_->setMinimumWidth(260);
    inputField_->setMaximumHeight(36);

    categoryCombo_ = new QComboBox(this);
    for (const auto& cat : kCategories)
        categoryCombo_->addItem(cat);
    categoryCombo_->setFixedWidth(160);
    categoryCombo_->setMaximumHeight(36);

    addBtn_ = new QPushButton("Add", this);
    addBtn_->setFixedSize(80, 36);
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border: none; "
        "border-radius: 8px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
        "QPushButton:pressed { background: #1d4ed8; }");

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setFixedSize(80, 36);
    clearBtn_->setStyleSheet(
        "QPushButton { background: #f1f5f9; color: #475569; border: 1px solid #e2e8f0; "
        "border-radius: 8px; font-weight: bold; }"
        "QPushButton:hover { background: #e2e8f0; }"
        "QPushButton:pressed { background: #cbd5e1; }");

    toolbar->addWidget(inputField_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();

    infoLabel_ = new QLabel("Backlog: 0 papers | Urgent: 0 | Avg difficulty: 0.0", this);
    infoLabel_->setStyleSheet("color: #64748b; font-size: 13px; padding: 4px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);

    // Canvas area -- the paintEvent draws into the remaining space
    setMinimumSize(800, 480);

    // Connections
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingBacklog::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingBacklog::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperReadingBacklog::onAdd);
}
