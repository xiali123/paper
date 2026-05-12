#include "workspace/PaperIncidentBoard.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QFontMetrics>
#include <algorithm>
#include <qnumeric.h>

namespace {
static const QColor kPalette[] = {
    QColor("#3b82f6"),
    QColor("#16a34a"),
    QColor("#d97706"),
    QColor("#dc2626"),
    QColor("#7c3aed"),
};
static constexpr int kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);

static const QString kSeverities[] = {
    QStringLiteral("low"),
    QStringLiteral("medium"),
    QStringLiteral("high"),
    QStringLiteral("critical"),
};

static const QString kAssignees[] = {
    QStringLiteral("Alice"),
    QStringLiteral("Bob"),
    QStringLiteral("Carol"),
    QStringLiteral("Dave"),
    QStringLiteral("Eve"),
};

static const QString kStatuses[] = {
    QStringLiteral("open"),
    QStringLiteral("in-progress"),
    QStringLiteral("resolved"),
};

static const QString kCategories[] = {
    QStringLiteral("Bug"),
    QStringLiteral("Performance"),
    QStringLiteral("Security"),
    QStringLiteral("Feature"),
    QStringLiteral("Infra"),
};

QString randomElement(const QString* arr, int count) {
    return arr[QRandomGenerator::global()->bounded(count)];
}

int paletteIndex(int id) {
    return id % kPaletteSize;
}
} // anonymous namespace

PaperIncidentBoard::PaperIncidentBoard(QWidget* parent)
    : QWidget(parent)
    , settings_(QStringLiteral("PaperCrawler"), QStringLiteral("IncidentBoard"))
    , logBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperIncidentBoard::setupUI() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // --- Input row ---
    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(6);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("Enter incident title..."));
    inputField_->setMinimumWidth(260);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperIncidentBoard::onLog);
    inputRow->addWidget(inputField_);

    categoryCombo_ = new QComboBox(this);
    const int catCount = sizeof(kCategories) / sizeof(kCategories[0]);
    for (int i = 0; i < catCount; ++i) {
        categoryCombo_->addItem(kCategories[i]);
    }
    inputRow->addWidget(categoryCombo_);

    logBtn_ = new QPushButton(QStringLiteral("Log Incident"), this);
    connect(logBtn_, &QPushButton::clicked, this, &PaperIncidentBoard::onLog);
    inputRow->addWidget(logBtn_);

    clearBtn_ = new QPushButton(QStringLiteral("Clear All"), this);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperIncidentBoard::onClear);
    inputRow->addWidget(clearBtn_);

    root->addLayout(inputRow);

    // --- Info label ---
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    root->addWidget(infoLabel_);
    updateInfo();

    // Reserve vertical stretch for the painted area
    root->addStretch(1);

    setMinimumSize(640, 480);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PaperIncidentBoard::addEntry(const IncidentEntry& entry) {
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<IncidentEntry> PaperIncidentBoard::entries() const {
    return entries_;
}

int PaperIncidentBoard::resolvedCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.resolved) {
            ++count;
        }
    }
    return count;
}

qreal PaperIncidentBoard::avgMttr() const {
    if (entries_.isEmpty()) {
        return 0.0;
    }
    qreal sum = 0.0;
    int n = 0;
    for (const auto& e : entries_) {
        sum += e.mttr;
        ++n;
    }
    return n > 0 ? sum / static_cast<qreal>(n) : 0.0;
}

QMap<QString, int> PaperIncidentBoard::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperIncidentBoard::onLog() {
    const QString title = inputField_->text().trimmed();
    if (title.isEmpty()) {
        return;
    }

    const int id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    const QString category = categoryCombo_->currentText();
    const QString severity = randomElement(kSeverities, sizeof(kSeverities) / sizeof(kSeverities[0]));
    const QString assignee = randomElement(kAssignees, sizeof(kAssignees) / sizeof(kAssignees[0]));
    const qreal mttr = static_cast<qreal>(QRandomGenerator::global()->bounded(1, 721)) / 10.0; // 0.1 - 72.0 h
    const QString status = randomElement(kStatuses, sizeof(kStatuses) / sizeof(kStatuses[0]));
    const bool resolved = (status == QStringLiteral("resolved"));
    const QColor color = kPalette[paletteIndex(id)];

    IncidentEntry entry;
    entry.id        = id;
    entry.title     = title;
    entry.category  = category;
    entry.severity  = severity;
    entry.assignee  = assignee;
    entry.mttr      = mttr;
    entry.status    = status;
    entry.resolved  = resolved;
    entry.color     = color;

    entries_.append(entry);

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();

    emit incidentLogged(id, mttr);
}

void PaperIncidentBoard::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperIncidentBoard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Background
    p.fillRect(rect(), QColor("#1e1e2e"));

    // Layout: top area consumed by widgets; paint area starts below infoLabel_.
    const int paintTop = infoLabel_ ? infoLabel_->geometry().bottom() + 10 : 120;
    const int paintH = h - paintTop - 10;
    if (paintH < 60) {
        return;
    }

    // Left half  -> incident list
    // Right half -> category chart (top) + stats (bottom)
    const int halfW = w / 2;

    QRect listRect(10, paintTop, halfW - 20, paintH);
    QRect chartRect(halfW + 10, paintTop, halfW - 20, paintH * 2 / 3);
    QRect statsRect(halfW + 10, paintTop + paintH * 2 / 3 + 10, halfW - 20, paintH / 3 - 10);

    drawIncidentList(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperIncidentBoard::drawIncidentList(QPainter& p, const QRect& rect) {
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 30, 46, 200));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#cdd6f4"));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Incident List (%1)").arg(entries_.size()));

    // Rows
    QFont rowFont = font();
    rowFont.setPointSize(rowFont.pointSize());
    p.setFont(rowFont);

    const int rowH = 28;
    const int topPad = 32;
    const int visibleRows = (rect.height() - topPad - 8) / rowH;

    // Show most recent first
    const int total = entries_.size();
    const int startIdx = total > visibleRows ? total - visibleRows : 0;

    for (int i = startIdx; i < total; ++i) {
        const auto& e = entries_[i];
        const int y = rect.y() + topPad + (i - startIdx) * rowH;
        if (y + rowH > rect.bottom() - 4) {
            break;
        }

        // Severity color bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(QRect(rect.x() + 8, y + 4, 4, rowH - 8), 2, 2);

        // Text line: id, title, status
        p.setPen(QColor("#cdd6f4"));
        const QString line = QStringLiteral("#%1  %2  [%3]  %4")
                                 .arg(e.id)
                                 .arg(e.title.length() > 20 ? e.title.left(20) + QStringLiteral("...") : e.title)
                                 .arg(e.severity)
                                 .arg(e.status);
        p.drawText(QPoint(rect.x() + 18, y + rowH - 8), line);
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#6c7086"));
        p.drawText(rect, Qt::AlignCenter, QStringLiteral("No incidents logged yet"));
    }
}

void PaperIncidentBoard::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 30, 46, 200));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#cdd6f4"));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Category Breakdown"));

    const auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont hintFont = font();
        p.setFont(hintFont);
        p.setPen(QColor("#6c7086"));
        p.drawText(rect, Qt::AlignCenter, QStringLiteral("No data"));
        return;
    }

    // Determine max for scaling
    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        maxCount = qMax(maxCount, it.value());
    }
    if (maxCount == 0) {
        maxCount = 1;
    }

    QFont rowFont = font();
    p.setFont(rowFont);

    const int barAreaLeft = rect.x() + 80;
    const int barMaxW = rect.width() - 100;
    const int rowH = 24;
    const int topPad = 34;
    int row = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        const int y = rect.y() + topPad + row * rowH;
        if (y + rowH > rect.bottom() - 4) {
            break;
        }

        // Label
        p.setPen(QColor("#cdd6f4"));
        p.drawText(QRect(rect.x() + 10, y, 66, rowH), Qt::AlignRight | Qt::AlignVCenter, it.key());

        // Bar
        const int barW = static_cast<int>(static_cast<qreal>(it.value()) / maxCount * barMaxW);
        const QColor barColor = kPalette[row % kPaletteSize];
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(QRect(barAreaLeft, y + 3, qMax(barW, 4), rowH - 6), 3, 3);

        // Count text
        p.setPen(QColor("#cdd6f4"));
        p.drawText(QPoint(barAreaLeft + barW + 8, y + rowH - 6), QString::number(it.value()));

        ++row;
    }
}

void PaperIncidentBoard::drawStats(QPainter& p, const QRect& rect) {
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 30, 46, 200));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#cdd6f4"));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Statistics"));

    QFont bodyFont = font();
    p.setFont(bodyFont);
    p.setPen(QColor("#a6adc8"));

    const int lh = 20;
    const int topPad = 30;
    const int x = rect.x() + 14;
    int y = rect.y() + topPad;

    // Total incidents
    p.drawText(QPoint(x, y), QStringLiteral("Total: %1").arg(entries_.size()));
    y += lh;

    // Resolved count
    const int resolved = resolvedCount();
    p.drawText(QPoint(x, y), QStringLiteral("Resolved: %1").arg(resolved));
    y += lh;

    // Open count
    p.drawText(QPoint(x, y), QStringLiteral("Open/In-Progress: %1").arg(entries_.size() - resolved));
    y += lh;

    // Average MTTR
    p.drawText(QPoint(x, y), QStringLiteral("Avg MTTR: %1 h").arg(avgMttr(), 0, 'f', 1));
    y += lh;

    // Resolution rate
    const qreal rate = entries_.isEmpty() ? 0.0
                                          : static_cast<qreal>(resolved) / static_cast<qreal>(entries_.size()) * 100.0;
    p.drawText(QPoint(x, y), QStringLiteral("Resolution: %1%").arg(rate, 0, 'f', 1));
}

// ---------------------------------------------------------------------------
// Info label
// ---------------------------------------------------------------------------

void PaperIncidentBoard::updateInfo() {
    if (!infoLabel_) {
        return;
    }
    const int total = entries_.size();
    const int resolved = resolvedCount();
    const qreal mttr = avgMttr();

    infoLabel_->setText(
        QStringLiteral("Incidents: %1  |  Resolved: %2  |  Avg MTTR: %3 h")
            .arg(total)
            .arg(resolved)
            .arg(mttr, 0, 'f', 1));
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void PaperIncidentBoard::loadSettings() {
    settings_.beginGroup(QStringLiteral("entries"));
    const int count = settings_.value(QStringLiteral("count"), 0).toInt();
    entries_.clear();
    entries_.reserve(count);
    for (int i = 0; i < count; ++i) {
        const QString prefix = QStringLiteral("e%1_").arg(i);
        IncidentEntry e;
        e.id       = settings_.value(prefix + QStringLiteral("id"), 0).toInt();
        e.title    = settings_.value(prefix + QStringLiteral("title")).toString();
        e.category = settings_.value(prefix + QStringLiteral("category")).toString();
        e.severity = settings_.value(prefix + QStringLiteral("severity")).toString();
        e.assignee = settings_.value(prefix + QStringLiteral("assignee")).toString();
        e.mttr     = settings_.value(prefix + QStringLiteral("mttr"), 0.0).toDouble();
        e.status   = settings_.value(prefix + QStringLiteral("status")).toString();
        e.resolved = settings_.value(prefix + QStringLiteral("resolved"), false).toBool();
        e.color    = QColor(settings_.value(prefix + QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperIncidentBoard::saveSettings() {
    settings_.beginGroup(QStringLiteral("entries"));
    settings_.remove(QString()); // clear group
    settings_.setValue(QStringLiteral("count"), entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        const QString prefix = QStringLiteral("e%1_").arg(i);
        settings_.setValue(prefix + QStringLiteral("id"), e.id);
        settings_.setValue(prefix + QStringLiteral("title"), e.title);
        settings_.setValue(prefix + QStringLiteral("category"), e.category);
        settings_.setValue(prefix + QStringLiteral("severity"), e.severity);
        settings_.setValue(prefix + QStringLiteral("assignee"), e.assignee);
        settings_.setValue(prefix + QStringLiteral("mttr"), e.mttr);
        settings_.setValue(prefix + QStringLiteral("status"), e.status);
        settings_.setValue(prefix + QStringLiteral("resolved"), e.resolved);
        settings_.setValue(prefix + QStringLiteral("color"), e.color.name());
    }
    settings_.endGroup();
    settings_.sync();
}
