#include "citation/PaperCitationDiff.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <QFont>
#include <algorithm>
#include <cmath>

PaperCitationDiff::PaperCitationDiff(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "CitationDiff")
{
    setupUI();
    loadSettings();
}

void PaperCitationDiff::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel: controls
    auto* leftWidget = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Category combo
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem("All Categories");
    categoryCombo_->addItem("Journal");
    categoryCombo_->addItem("Conference");
    categoryCombo_->addItem("Preprint");
    categoryCombo_->addItem("Book");
    categoryCombo_->addItem("Thesis");
    leftLayout->addWidget(categoryCombo_);

    // Input field
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    leftLayout->addWidget(inputField_);

    // Buttons row
    auto* btnLayout = new QHBoxLayout();
    diffBtn_ = new QPushButton("Diff", this);
    clearBtn_ = new QPushButton("Clear", this);
    btnLayout->addWidget(diffBtn_);
    btnLayout->addWidget(clearBtn_);
    leftLayout->addLayout(btnLayout);

    // Info label
    infoLabel_ = new QLabel("Changes: 0 | Added: 0 | Avg Magnitude: 0.0", this);
    leftLayout->addWidget(infoLabel_);

    mainLayout->addWidget(leftWidget);

    // Right stretch for painted area
    mainLayout->addStretch(1);

    // Connections
    connect(diffBtn_, &QPushButton::clicked, this, &PaperCitationDiff::onDiff);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationDiff::onClear);

    setMinimumSize(800, 500);
}

void PaperCitationDiff::loadSettings()
{
    settings_.beginGroup("CitationDiff");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        CitationDiffEntry entry;
        entry.id = settings_.value("id", i).toInt();
        entry.paper = settings_.value("paper").toString();
        entry.category = settings_.value("category").toString();
        entry.change = settings_.value("change", "added").toString();
        entry.magnitude = settings_.value("magnitude", 0.0).toReal();
        entry.citations = settings_.value("citations", 0).toInt();
        entry.added = settings_.value("added", true).toBool();
        entry.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperCitationDiff::saveSettings()
{
    settings_.beginGroup("CitationDiff");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_.at(i);
        settings_.setValue("id", entry.id);
        settings_.setValue("paper", entry.paper);
        settings_.setValue("category", entry.category);
        settings_.setValue("change", entry.change);
        settings_.setValue("magnitude", entry.magnitude);
        settings_.setValue("citations", entry.citations);
        settings_.setValue("added", entry.added);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperCitationDiff::onDiff()
{
    QString paper = inputField_->text().trimmed();
    if (paper.isEmpty()) return;

    QString category = categoryCombo_->currentText();

    qreal magnitude = QRandomGenerator::global()->bounded(101);
    int citations = QRandomGenerator::global()->bounded(501);
    bool added = QRandomGenerator::global()->bounded(2) == 0;

    QStringList changeTypes = {"added", "removed", "modified"};
    QString change = changeTypes.at(QRandomGenerator::global()->bounded(3));

    QColor color;
    if (change == "added") {
        color = QColor("#16a34a");
    } else if (change == "removed") {
        color = QColor("#dc2626");
    } else {
        color = QColor("#d97706");
    }

    CitationDiffEntry entry;
    entry.id = entries_.size();
    entry.paper = paper;
    entry.category = category;
    entry.change = change;
    entry.magnitude = magnitude;
    entry.citations = citations;
    entry.added = added;
    entry.color = color;
    entries_.append(entry);

    emit diffFound(entry.id, entry.magnitude);
    saveSettings();
    updateInfo();
    update();
}

void PaperCitationDiff::onClear()
{
    entries_.clear();
    inputField_->clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperCitationDiff::addEntry(const CitationDiffEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<CitationDiffEntry> PaperCitationDiff::entries() const
{
    return entries_;
}

int PaperCitationDiff::addedCount() const
{
    int count = 0;
    for (const auto& entry : entries_) {
        if (entry.change == "added") ++count;
    }
    return count;
}

qreal PaperCitationDiff::avgMagnitude() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& entry : entries_) {
        sum += entry.magnitude;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperCitationDiff::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& entry : entries_) {
        counts[entry.category]++;
    }
    return counts;
}

void PaperCitationDiff::updateInfo()
{
    int total = entries_.size();
    int added = addedCount();
    qreal avg = avgMagnitude();
    infoLabel_->setText(QString("Changes: %1 | Added: %2 | Avg Magnitude: %3")
                            .arg(total)
                            .arg(added)
                            .arg(avg, 0, 'f', 1));
}

void PaperCitationDiff::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Reserve top area for controls (left panel widgets)
    int controlHeight = 160;
    int topMargin = controlHeight + 10;
    int availableHeight = h - topMargin;
    if (availableHeight < 100) availableHeight = 100;

    int colWidth = w / 3;

    QRect listRect(0, topMargin, colWidth, availableHeight);
    QRect chartRect(colWidth, topMargin, colWidth, availableHeight);
    QRect statsRect(colWidth * 2, topMargin, w - colWidth * 2, availableHeight);

    drawDiffList(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperCitationDiff::drawDiffList(QPainter& p, const QRect& rect)
{
    // Background
    p.fillRect(rect, QColor("#1e293b"));

    // Border
    p.setPen(QColor("#334155"));
    p.drawRect(rect);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    p.setPen(QColor("#3b82f6"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Citation Diff");

    // List entries
    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    int y = rect.top() + 35;
    int maxEntries = (rect.height() - 45) / 40;
    int startIdx = qMax(0, entries_.size() - maxEntries);

    for (int i = startIdx; i < entries_.size() && y < rect.bottom() - 10; ++i) {
        const auto& entry = entries_.at(i);
        int rowHeight = 36;

        // Magnitude bar background
        p.fillRect(rect.left() + 10, y, rect.width() - 20, 6, QColor("#334155"));

        // Magnitude bar fill
        int barWidth = static_cast<int>((rect.width() - 20) * (entry.magnitude / 100.0));
        p.fillRect(rect.left() + 10, y, barWidth, 6, entry.color);

        // Change type symbol
        QString symbol;
        QColor symbolColor;
        if (entry.change == "added") {
            symbol = "+";
            symbolColor = QColor("#16a34a");
        } else if (entry.change == "removed") {
            symbol = "-";
            symbolColor = QColor("#dc2626");
        } else {
            symbol = "~";
            symbolColor = QColor("#d97706");
        }

        p.setPen(symbolColor);
        QFont symFont = p.font();
        symFont.setBold(true);
        symFont.setPointSize(11);
        p.setFont(symFont);
        p.drawText(QRect(rect.left() + 10, y + 7, 16, 20), Qt::AlignCenter, symbol);

        // Paper title
        p.setFont(itemFont);
        p.setPen(QColor("#e2e8f0"));
        QString displayText = entry.paper;
        if (displayText.length() > 25) {
            displayText = displayText.left(22) + "...";
        }
        p.drawText(QRect(rect.left() + 28, y + 7, rect.width() - 80, 20),
                   Qt::AlignLeft | Qt::AlignVCenter, displayText);

        // Magnitude value
        p.setPen(QColor("#94a3b8"));
        p.drawText(QRect(rect.right() - 55, y + 7, 45, 20),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(entry.magnitude, 'f', 1));

        y += rowHeight;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#64748b"));
        p.setFont(itemFont);
        p.drawText(rect.adjusted(10, 0, -10, 0), Qt::AlignCenter, "No citation diffs yet");
    }
}

void PaperCitationDiff::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Background
    p.fillRect(rect, QColor("#1e293b"));

    // Border
    p.setPen(QColor("#334155"));
    p.drawRect(rect);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    p.setPen(QColor("#7c3aed"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    // Category chart
    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont itemFont = p.font();
        itemFont.setBold(false);
        itemFont.setPointSize(9);
        p.setFont(itemFont);
        p.setPen(QColor("#64748b"));
        p.drawText(rect.adjusted(10, 0, -10, 0), Qt::AlignCenter, "No categories yet");
        return;
    }

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    int y = rect.top() + 40;
    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount) maxCount = it.value();
    }
    if (maxCount == 0) maxCount = 1;

    QList<QColor> barColors = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
                               QColor("#dc2626"), QColor("#7c3aed")};
    int colorIdx = 0;
    int barMaxWidth = rect.width() - 130;

    for (auto it = counts.constBegin(); it != counts.constEnd() && y < rect.bottom() - 20; ++it) {
        // Category label
        p.setPen(QColor("#e2e8f0"));
        p.drawText(QRect(rect.left() + 10, y, 80, 22), Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar background
        p.fillRect(rect.left() + 95, y + 3, barMaxWidth, 16, QColor("#334155"));

        // Bar fill
        int barW = static_cast<int>(barMaxWidth * (static_cast<qreal>(it.value()) / maxCount));
        QColor barColor = barColors.at(colorIdx % barColors.size());
        p.fillRect(rect.left() + 95, y + 3, barW, 16, barColor);

        // Count label
        p.setPen(QColor("#94a3b8"));
        p.drawText(QRect(rect.left() + 95 + barMaxWidth + 5, y, 30, 22),
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += 30;
        ++colorIdx;
    }
}

void PaperCitationDiff::drawStats(QPainter& p, const QRect& rect)
{
    // Background
    p.fillRect(rect, QColor("#1e293b"));

    // Border
    p.setPen(QColor("#334155"));
    p.drawRect(rect);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    p.setPen(QColor("#3b82f6"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(10);
    p.setFont(itemFont);

    int y = rect.top() + 45;
    int lineHeight = 35;
    int centerX = rect.left() + rect.width() / 2;

    // Total changes
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Total Changes:");
    p.setPen(QColor("#e2e8f0"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(entries_.size()));

    y += lineHeight;

    // Divider
    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + 15, y, rect.right() - 15, y);

    y += 10;

    // Added count
    p.setPen(QColor("#94a3b8"));
    p.setFont(itemFont);
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Added:");
    p.setPen(QColor("#16a34a"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(addedCount()));

    y += lineHeight;

    // Divider
    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + 15, y, rect.right() - 15, y);

    y += 10;

    // Removed count
    int removedCount = 0;
    for (const auto& e : entries_) {
        if (e.change == "removed") ++removedCount;
    }
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Removed:");
    p.setPen(QColor("#dc2626"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(removedCount));

    y += lineHeight;

    // Divider
    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + 15, y, rect.right() - 15, y);

    y += 10;

    // Modified count
    int modifiedCount = 0;
    for (const auto& e : entries_) {
        if (e.change == "modified") ++modifiedCount;
    }
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Modified:");
    p.setPen(QColor("#d97706"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(modifiedCount));

    y += lineHeight;

    // Divider
    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + 15, y, rect.right() - 15, y);

    y += 10;

    // Average magnitude
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Avg Magnitude:");
    p.setPen(QColor("#3b82f6"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(avgMagnitude(), 'f', 1));

    y += lineHeight;

    // Divider
    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + 15, y, rect.right() - 15, y);

    y += 10;

    // Total citations
    int totalCitations = 0;
    for (const auto& e : entries_) {
        totalCitations += e.citations;
    }
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Total Citations:");
    p.setPen(QColor("#7c3aed"));
    p.drawText(QRect(rect.left() + 15, y, rect.width() - 30, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(totalCitations));
}
