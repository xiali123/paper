#include "citation/PaperCitationMerge.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <QFont>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

PaperCitationMerge::PaperCitationMerge(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "CitationMerge")
{
    setupUI();
    loadSettings();
}

void PaperCitationMerge::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Top controls row
    auto* controlLayout = new QHBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem("All");
    categoryCombo_->addItem("Exact");
    categoryCombo_->addItem("Fuzzy");
    categoryCombo_->addItem("Semantic");
    categoryCombo_->addItem("Manual");
    controlLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Left==Right");
    controlLayout->addWidget(inputField_);

    mergeBtn_ = new QPushButton("Merge", this);
    controlLayout->addWidget(mergeBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    controlLayout->addWidget(clearBtn_);

    mainLayout->addLayout(controlLayout);

    infoLabel_ = new QLabel("Entries: 0 | Merged: 0 | Avg Similarity: 0.0", this);
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);

    connect(mergeBtn_, &QPushButton::clicked, this, &PaperCitationMerge::onMerge);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationMerge::onClear);

    setMinimumSize(850, 520);
}

void PaperCitationMerge::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Reserve top area for controls
    int controlHeight = 90;
    int topMargin = controlHeight;
    int availableHeight = h - topMargin;
    if (availableHeight < 100) availableHeight = 100;

    // Top half: merge view
    int mergeHeight = availableHeight * 55 / 100;
    QRect mergeRect(0, topMargin, w, mergeHeight);

    // Bottom half split into chart (left) and stats (right)
    int bottomY = topMargin + mergeHeight;
    int bottomHeight = availableHeight - mergeHeight;
    int colWidth = w / 2;
    QRect chartRect(0, bottomY, colWidth, bottomHeight);
    QRect statsRect(colWidth, bottomY, w - colWidth, bottomHeight);

    drawMergeView(p, mergeRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperCitationMerge::drawMergeView(QPainter& p, const QRect& rect)
{
    // Background
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 6, 6);
    p.fillPath(bgPath, QColor("#1e293b"));

    // Border
    p.setPen(QPen(QColor("#334155"), 1));
    p.drawPath(bgPath);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    p.setPen(QColor("#3b82f6"));
    p.drawText(rect.adjusted(14, 10, -10, 0), Qt::AlignLeft | Qt::AlignTop,
               "Citation Merge View");

    if (entries_.isEmpty()) {
        QFont emptyFont = p.font();
        emptyFont.setBold(false);
        emptyFont.setPointSize(10);
        p.setFont(emptyFont);
        p.setPen(QColor("#64748b"));
        p.drawText(rect.adjusted(10, 0, -10, 0), Qt::AlignCenter,
                   "No merge entries yet. Enter Left==Right to begin.");
        return;
    }

    // Draw entry pairs
    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    int y = rect.top() + 38;
    int rowHeight = 48;
    int maxVisible = (rect.height() - 48) / rowHeight;
    int startIdx = qMax(0, entries_.size() - maxVisible);

    int leftColW = (rect.width() - 180) / 2;
    int rightColX = rect.left() + 14 + leftColW + 60;
    int rightColW = leftColW;

    for (int i = startIdx; i < entries_.size() && y + rowHeight <= rect.bottom() - 6; ++i) {
        const auto& entry = entries_.at(i);
        int rowX = rect.left() + 14;
        int rowW = rect.width() - 28;

        // Row background
        QPainterPath rowPath;
        rowPath.addRoundedRect(QRect(rowX, y, rowW, rowHeight - 4), 4, 4);
        if (entry.merged) {
            p.fillPath(rowPath, QColor("#1a3a2a"));
        } else {
            p.fillPath(rowPath, QColor("#253347"));
        }
        p.setPen(QPen(QColor("#334155"), 1));
        p.drawPath(rowPath);

        // Left citation
        p.setPen(QColor("#e2e8f0"));
        QFontMetrics fm(itemFont);
        QString leftText = fm.elidedText(entry.left, Qt::ElideRight, leftColW - 10);
        p.drawText(QRect(rowX + 8, y + 4, leftColW, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   leftText);

        // Separator "=="
        p.setPen(entry.color);
        QFont sepFont = itemFont;
        sepFont.setBold(true);
        sepFont.setPointSize(10);
        p.setFont(sepFont);
        p.drawText(QRect(rowX + leftColW + 8, y + 4, 44, 20), Qt::AlignCenter, "==");
        p.setFont(itemFont);

        // Right citation
        p.setPen(QColor("#e2e8f0"));
        QString rightText = fm.elidedText(entry.right, Qt::ElideRight, rightColW - 10);
        p.drawText(QRect(rightColX, y + 4, rightColW, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   rightText);

        // Similarity bar (below the text row)
        int barY = y + 26;
        int barMaxWidth = rowW - 100;
        int barH = 8;

        // Bar background
        QPainterPath barBg;
        barBg.addRoundedRect(QRect(rowX + 8, barY, barMaxWidth, barH), 3, 3);
        p.fillPath(barBg, QColor("#334155"));

        // Bar fill based on similarity
        int barFillW = static_cast<int>(barMaxWidth * qBound(0.0, entry.similarity, 100.0) / 100.0);
        if (barFillW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRect(rowX + 8, barY, barFillW, barH), 3, 3);
            p.fillPath(barFill, entry.color);
        }

        // Similarity value
        p.setPen(QColor("#94a3b8"));
        p.drawText(QRect(rowX + barMaxWidth + 14, barY - 3, 50, 14), Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(entry.similarity, 'f', 1) + "%");

        // Conflict count badge
        if (entry.conflicts > 0) {
            int badgeX = rect.right() - 70;
            QPainterPath badgePath;
            badgePath.addRoundedRect(QRect(badgeX, y + 4, 26, 18), 9, 9);
            p.fillPath(badgePath, QColor("#dc2626"));
            p.setPen(Qt::white);
            QFont badgeFont = itemFont;
            badgeFont.setPointSize(8);
            badgeFont.setBold(true);
            p.setFont(badgeFont);
            p.drawText(QRect(badgeX, y + 4, 26, 18), Qt::AlignCenter,
                       QString::number(entry.conflicts));
            p.setFont(itemFont);
        }

        // Merged badge
        if (entry.merged) {
            int badgeX = rect.right() - 38;
            QPainterPath mergedPath;
            mergedPath.addRoundedRect(QRect(badgeX, y + 6, 30, 14), 7, 7);
            p.fillPath(mergedPath, QColor("#16a34a"));
            p.setPen(Qt::white);
            QFont mergedFont = itemFont;
            mergedFont.setPointSize(7);
            mergedFont.setBold(true);
            p.setFont(mergedFont);
            p.drawText(QRect(badgeX, y + 6, 30, 14), Qt::AlignCenter, "OK");
            p.setFont(itemFont);
        }

        y += rowHeight;
    }
}

void PaperCitationMerge::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Background
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 6, 6);
    p.fillPath(bgPath, QColor("#1e293b"));

    p.setPen(QPen(QColor("#334155"), 1));
    p.drawPath(bgPath);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    p.setPen(QColor("#7c3aed"));
    p.drawText(rect.adjusted(14, 10, -10, 0), Qt::AlignLeft | Qt::AlignTop,
               "Category Breakdown");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont itemFont = p.font();
        itemFont.setBold(false);
        itemFont.setPointSize(9);
        p.setFont(itemFont);
        p.setPen(QColor("#64748b"));
        p.drawText(rect.adjusted(10, 0, -10, 0), Qt::AlignCenter, "No data yet");
        return;
    }

    // Draw pie chart
    int pieSize = qMin(rect.width(), rect.height()) - 80;
    if (pieSize < 60) pieSize = 60;
    int pieX = rect.left() + (rect.width() - pieSize) / 2;
    int pieY = rect.top() + 40;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        total += it.value();
    }
    if (total == 0) total = 1;

    // Category-to-color mapping
    QMap<QString, QColor> catColors;
    catColors["Exact"] = QColor("#3b82f6");
    catColors["Fuzzy"] = QColor("#16a34a");
    catColors["Semantic"] = QColor("#7c3aed");
    catColors["Manual"] = QColor("#d97706");
    QList<QColor> fallbackColors = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#7c3aed"),
                                    QColor("#d97706"), QColor("#ec4899"), QColor("#06b6d4")};
    int fallbackIdx = 0;

    qreal startAngle = 0.0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal span = 360.0 * it.value() / total;
        QColor sliceColor = catColors.value(it.key(), fallbackColors.at(fallbackIdx % fallbackColors.size()));
        ++fallbackIdx;

        QPainterPath slicePath;
        slicePath.moveTo(pieX + pieSize / 2, pieY + pieSize / 2);
        slicePath.arcTo(QRect(pieX, pieY, pieSize, pieSize),
                        static_cast<int>(startAngle * 16),
                        static_cast<int>(span * 16));
        slicePath.closeSubpath();
        p.fillPath(slicePath, sliceColor);
        p.setPen(QPen(QColor("#1e293b"), 2));
        p.drawPath(slicePath);

        // Label on slice
        if (span > 15.0) {
            qreal midAngle = (startAngle + span / 2.0) * M_PI / 180.0;
            int labelR = pieSize / 3;
            int lx = pieX + pieSize / 2 + static_cast<int>(labelR * std::cos(midAngle));
            int ly = pieY + pieSize / 2 - static_cast<int>(labelR * std::sin(midAngle));
            QFont labelFont = p.font();
            labelFont.setPointSize(8);
            labelFont.setBold(true);
            p.setFont(labelFont);
            p.setPen(Qt::white);
            p.drawText(QRect(lx - 30, ly - 8, 60, 16), Qt::AlignCenter,
                       it.key() + "\n" + QString::number(it.value()));
        }

        startAngle += span;
    }

    // Legend below pie
    QFont legendFont = p.font();
    legendFont.setPointSize(8);
    legendFont.setBold(false);
    p.setFont(legendFont);

    int legendY = pieY + pieSize + 12;
    int legendX = rect.left() + 14;
    for (auto it = counts.constBegin(); it != counts.constEnd() && legendY < rect.bottom() - 10; ++it) {
        QColor c = catColors.value(it.key(), QColor("#94a3b8"));
        p.fillRect(legendX, legendY + 2, 10, 10, c);
        p.setPen(QColor("#e2e8f0"));
        p.drawText(QRect(legendX + 16, legendY, 120, 14), Qt::AlignLeft | Qt::AlignVCenter,
                   it.key() + ": " + QString::number(it.value()));
        legendX += 130;
        if (legendX + 130 > rect.right()) {
            legendX = rect.left() + 14;
            legendY += 18;
        }
    }
}

void PaperCitationMerge::drawStats(QPainter& p, const QRect& rect)
{
    // Background
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 6, 6);
    p.fillPath(bgPath, QColor("#1e293b"));

    p.setPen(QPen(QColor("#334155"), 1));
    p.drawPath(bgPath);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    p.setPen(QColor("#3b82f6"));
    p.drawText(rect.adjusted(14, 10, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(10);
    p.setFont(itemFont);

    int y = rect.top() + 45;
    int lineHeight = 36;
    int marginX = 18;
    int contentW = rect.width() - marginX * 2;

    // Total entries
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Total Entries:");
    p.setPen(QColor("#e2e8f0"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(entries_.size()));
    y += lineHeight;

    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + marginX, y - lineHeight / 2 + 4,
               rect.right() - marginX, y - lineHeight / 2 + 4);

    // Merged count
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Merged:");
    p.setPen(QColor("#16a34a"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(mergedCount()));
    y += lineHeight;

    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + marginX, y - lineHeight / 2 + 4,
               rect.right() - marginX, y - lineHeight / 2 + 4);

    // Unmerged count
    int unmerged = entries_.size() - mergedCount();
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Unmerged:");
    p.setPen(QColor("#d97706"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(unmerged));
    y += lineHeight;

    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + marginX, y - lineHeight / 2 + 4,
               rect.right() - marginX, y - lineHeight / 2 + 4);

    // Average similarity
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Avg Similarity:");
    p.setPen(QColor("#3b82f6"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter,
               QString::number(avgSimilarity(), 'f', 1) + "%");
    y += lineHeight;

    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + marginX, y - lineHeight / 2 + 4,
               rect.right() - marginX, y - lineHeight / 2 + 4);

    // Total conflicts
    int totalConflicts = 0;
    for (const auto& e : entries_) {
        totalConflicts += e.conflicts;
    }
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Total Conflicts:");
    p.setPen(QColor("#dc2626"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter, QString::number(totalConflicts));
    y += lineHeight;

    p.setPen(QColor("#334155"));
    p.drawLine(rect.left() + marginX, y - lineHeight / 2 + 4,
               rect.right() - marginX, y - lineHeight / 2 + 4);

    // Merge rate
    qreal mergeRate = entries_.isEmpty() ? 0.0
                                         : 100.0 * mergedCount() / entries_.size();
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignLeft | Qt::AlignVCenter, "Merge Rate:");
    p.setPen(QColor("#7c3aed"));
    p.drawText(QRect(rect.left() + marginX, y, contentW, lineHeight),
               Qt::AlignRight | Qt::AlignVCenter,
               QString::number(mergeRate, 'f', 1) + "%");
}

void PaperCitationMerge::onMerge()
{
    QString input = inputField_->text().trimmed();
    if (input.isEmpty()) return;

    // Parse "Left==Right" format
    QString leftCitation;
    QString rightCitation;
    int sepIdx = input.indexOf("==");
    if (sepIdx > 0) {
        leftCitation = input.left(sepIdx).trimmed();
        rightCitation = input.mid(sepIdx + 2).trimmed();
    } else {
        leftCitation = input;
        rightCitation = input;
    }

    QString category = categoryCombo_->currentText();

    // Generate similarity based on category
    qreal similarity = 0.0;
    QColor color;
    if (category == "Exact") {
        similarity = 95.0 + QRandomGenerator::global()->bounded(60) / 10.0;
        color = QColor("#3b82f6");
    } else if (category == "Fuzzy") {
        similarity = 60.0 + QRandomGenerator::global()->bounded(350) / 10.0;
        color = QColor("#16a34a");
    } else if (category == "Semantic") {
        similarity = 40.0 + QRandomGenerator::global()->bounded(500) / 10.0;
        color = QColor("#7c3aed");
    } else if (category == "Manual") {
        similarity = 30.0 + QRandomGenerator::global()->bounded(700) / 10.0;
        color = QColor("#d97706");
    } else {
        // "All" category picks a random match type
        int typeIdx = QRandomGenerator::global()->bounded(4);
        QStringList types = {"Exact", "Fuzzy", "Semantic", "Manual"};
        QList<QColor> typeColors = {QColor("#3b82f6"), QColor("#16a34a"),
                                    QColor("#7c3aed"), QColor("#d97706")};
        QList<qreal> minSim = {95.0, 60.0, 40.0, 30.0};
        QList<qreal> rangeSim = {6.0, 35.0, 50.0, 70.0};
        category = types.at(typeIdx);
        color = typeColors.at(typeIdx);
        similarity = minSim.at(typeIdx) +
                     QRandomGenerator::global()->bounded(static_cast<int>(rangeSim.at(typeIdx) * 10)) / 10.0;
    }

    similarity = qBound(0.0, similarity, 100.0);

    // Auto-merge if similarity >= 95
    bool merged = similarity >= 95.0;

    // Conflict count inversely related to similarity
    int conflicts = 0;
    if (similarity < 50.0) {
        conflicts = 3 + QRandomGenerator::global()->bounded(8);
    } else if (similarity < 80.0) {
        conflicts = 1 + QRandomGenerator::global()->bounded(4);
    } else if (similarity < 95.0) {
        conflicts = QRandomGenerator::global()->bounded(2);
    }

    CitationMergeEntry entry;
    entry.id = entries_.size();
    entry.left = leftCitation;
    entry.category = category;
    entry.right = rightCitation;
    entry.similarity = similarity;
    entry.conflicts = conflicts;
    entry.merged = merged;
    entry.color = color;
    entries_.append(entry);

    emit citationMerged(entry.id, entry.similarity);
    saveSettings();
    updateInfo();
    update();
}

void PaperCitationMerge::onClear()
{
    entries_.clear();
    inputField_->clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperCitationMerge::updateInfo()
{
    int total = entries_.size();
    int merged = mergedCount();
    qreal avg = avgSimilarity();
    infoLabel_->setText(QString("Entries: %1 | Merged: %2 | Avg Similarity: %3")
                            .arg(total)
                            .arg(merged)
                            .arg(avg, 0, 'f', 1));
}

void PaperCitationMerge::loadSettings()
{
    settings_.beginGroup("CitationMerge");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        CitationMergeEntry entry;
        entry.id = settings_.value("id", i).toInt();
        entry.left = settings_.value("left").toString();
        entry.category = settings_.value("category").toString();
        entry.right = settings_.value("right").toString();
        entry.similarity = settings_.value("similarity", 0.0).toReal();
        entry.conflicts = settings_.value("conflicts", 0).toInt();
        entry.merged = settings_.value("merged", false).toBool();
        entry.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperCitationMerge::saveSettings()
{
    settings_.beginGroup("CitationMerge");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_.at(i);
        settings_.setValue("id", entry.id);
        settings_.setValue("left", entry.left);
        settings_.setValue("category", entry.category);
        settings_.setValue("right", entry.right);
        settings_.setValue("similarity", entry.similarity);
        settings_.setValue("conflicts", entry.conflicts);
        settings_.setValue("merged", entry.merged);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperCitationMerge::addEntry(const CitationMergeEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<CitationMergeEntry> PaperCitationMerge::entries() const
{
    return entries_;
}

int PaperCitationMerge::mergedCount() const
{
    int count = 0;
    for (const auto& entry : entries_) {
        if (entry.merged) ++count;
    }
    return count;
}

qreal PaperCitationMerge::avgSimilarity() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& entry : entries_) {
        sum += entry.similarity;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperCitationMerge::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& entry : entries_) {
        counts[entry.category]++;
    }
    return counts;
}
