#include "analysis/PaperKnowledgeExtractor.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <QDateTime>
#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kBarMaxWidth = 160.0;
constexpr int kEntryHeight = 56;
constexpr int kChartSize = 200;
constexpr int kMargin = 12;
constexpr int kSpacing = 8;

const QColor kBlue   = QColorLiteral("#3b82f6");
const QColor kGreen  = QColorLiteral("#16a34a");
const QColor kAmber  = QColorLiteral("#d97706");
const QColor kRed    = QColorLiteral("#dc2626");
const QColor kPurple = QColorLiteral("#7c3aed");

QColor nextColor(int index) {
    const QColor palette[] = {kBlue, kGreen, kAmber, kRed, kPurple};
    return palette[index % 5];
}
}

PaperKnowledgeExtractor::PaperKnowledgeExtractor(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope,
                QStringLiteral("PaperCrawler"), QStringLiteral("KnowledgeExtractor"))
{
    setupUI();
    loadSettings();
}

void PaperKnowledgeExtractor::setupUI() {
    auto* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    topLayout->setSpacing(kSpacing);

    auto* controlRow = new QHBoxLayout();
    controlRow->setSpacing(kSpacing);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Enter entity or knowledge text..."));
    inputField_->setMinimumWidth(240);
    controlRow->addWidget(inputField_);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({
        tr("Concept"), tr("Person"), tr("Organization"),
        tr("Location"), tr("Method"), tr("Result"), tr("Other")
    });
    categoryCombo_->setMinimumWidth(120);
    controlRow->addWidget(categoryCombo_);

    extractBtn_ = new QPushButton(tr("Extract"), this);
    extractBtn_->setStyleSheet(
        QStringLiteral("QPushButton{background:%1;color:#fff;padding:6px 18px;"
                        "border:none;border-radius:4px;font-weight:bold;}"
                        "QPushButton:hover{background:%2;}")
            .arg(kBlue.name(), QColor(kBlue).lighter(110).name()));
    controlRow->addWidget(extractBtn_);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setStyleSheet(
        QStringLiteral("QPushButton{background:%1;color:#fff;padding:6px 18px;"
                        "border:none;border-radius:4px;font-weight:bold;}"
                        "QPushButton:hover{background:%2;}")
            .arg(kRed.name(), QColor(kRed).lighter(110).name()));
    controlRow->addWidget(clearBtn_);

    controlRow->addStretch();
    topLayout->addLayout(controlRow);

    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet(QStringLiteral("color:%1;font-size:12px;").arg(kBlue.name()));
    topLayout->addWidget(infoLabel_);

    setMinimumHeight(480);

    connect(extractBtn_, &QPushButton::clicked, this, &PaperKnowledgeExtractor::onExtract);
    connect(clearBtn_,   &QPushButton::clicked, this, &PaperKnowledgeExtractor::onClear);
}

void PaperKnowledgeExtractor::addEntry(const KnowEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<KnowEntry> PaperKnowledgeExtractor::entries() const {
    return entries_;
}

int PaperKnowledgeExtractor::verifiedCount() const {
    return static_cast<int>(
        std::count_if(entries_.cbegin(), entries_.cend(),
                      [](const KnowEntry& e) { return e.verified; }));
}

qreal PaperKnowledgeExtractor::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperKnowledgeExtractor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperKnowledgeExtractor::onExtract() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QString category = categoryCombo_->currentText();

    // Parse: support "entity -> relation" or "entity - relation" or plain text
    QString entity = text;
    QString relation = tr("extracted from input");
    const QList<QPair<QString, QString>> delimiters = {
        {QStringLiteral(" -> "), QStringLiteral(" -> ")},
        {QStringLiteral(" - "),  QStringLiteral(" - ")},
        {QStringLiteral(" : "),  QStringLiteral(" : ")},
    };
    for (const auto& delim : delimiters) {
        int idx = text.indexOf(delim.first);
        if (idx > 0) {
            entity = text.left(idx).trimmed();
            relation = text.mid(idx + delim.first.length()).trimmed();
            break;
        }
    }

    KnowEntry entry;
    entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 1000000);
    entry.entity = entity;
    entry.category = category;
    entry.relation = relation;
    entry.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
    entry.frequency = 1.0 + QRandomGenerator::global()->bounded(30) / 10.0;
    entry.sources = 1 + QRandomGenerator::global()->bounded(10);
    entry.verified = entry.confidence >= 0.8;
    entry.color = nextColor(entries_.size());

    addEntry(entry);
    inputField_->clear();
    emit knowledgeExtracted(entry.id, entry.confidence);
}

void PaperKnowledgeExtractor::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperKnowledgeExtractor::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int topOffset = (inputField_->geometry().bottom() + infoLabel_->height() + kSpacing * 3);

    QRect listRect(kMargin, topOffset, w / 2 - 2 * kMargin, height() - topOffset - kMargin);
    QRect chartRect(w / 2, topOffset, w / 4 - kMargin, kChartSize + 2 * kMargin);
    QRect statsRect(w * 3 / 4, topOffset, w / 4 - kMargin, height() - topOffset - kMargin);

    drawKnowledgeList(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperKnowledgeExtractor::drawKnowledgeList(QPainter& p, const QRect& rect) {
    // Background panel
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 12));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(kBlue);
    p.drawText(rect.adjusted(kMargin, kMargin, 0, 0),
               Qt::AlignLeft | Qt::AlignTop, tr("Knowledge Entries"));

    QFont normalFont = font();
    normalFont.setPointSize(normalFont.pointSize() - 1);
    p.setFont(normalFont);

    int y = rect.top() + kMargin + 28;
    const int visibleCount = std::min(entries_.size(),
        (rect.height() - 40) / kEntryHeight);

    for (int i = 0; i < visibleCount; ++i) {
        const KnowEntry& e = entries_.at(i);
        QRect entryRect(rect.left() + kMargin, y,
                        rect.width() - 2 * kMargin, kEntryHeight - kSpacing);

        // Entry background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 8));
        p.drawRoundedRect(entryRect, 6, 6);

        // Color indicator
        p.setBrush(e.color);
        p.drawRoundedRect(QRect(entryRect.left(), entryRect.top() + 4, 4, entryRect.height() - 8), 2, 2);

        // Entity text
        p.setPen(QColor(240, 240, 240));
        QFont entityFont = normalFont;
        entityFont.setBold(true);
        p.setFont(entityFont);
        QRect textRect = entryRect.adjusted(12, 4, -kBarMaxWidth - 12, entryRect.height() / 2 - 4);
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, e.entity);

        // Relation text
        p.setFont(normalFont);
        p.setPen(QColor(180, 180, 180));
        QRect relRect = entryRect.adjusted(12, entryRect.height() / 2, -kBarMaxWidth - 12, -4);
        QString relDisplay = e.relation.length() > 40
            ? e.relation.left(37) + QStringLiteral("...")
            : e.relation;
        p.drawText(relRect, Qt::AlignLeft | Qt::AlignVCenter, relDisplay);

        // Confidence bar background
        qreal barX = entryRect.right() - kBarMaxWidth;
        qreal barY = entryRect.top() + (entryRect.height() - 10) / 2.0;
        QRectF barBg(barX, barY, kBarMaxWidth, 10);
        p.setBrush(QColor(255, 255, 255, 30));
        p.drawRoundedRect(barBg, 4, 4);

        // Confidence bar fill
        QColor barColor = e.confidence >= 0.8 ? kGreen
                        : e.confidence >= 0.6 ? kAmber
                        : kRed;
        QRectF barFill(barX, barY, kBarMaxWidth * e.confidence, 10);
        p.setBrush(barColor);
        p.drawRoundedRect(barFill, 4, 4);

        // Confidence percentage
        p.setPen(QColor(200, 200, 200));
        p.setFont(normalFont);
        p.drawText(QRectF(barX, barY - 14, kBarMaxWidth, 14),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QStringLiteral("%1%").arg(e.confidence * 100, 0, 'f', 0));

        // Verified badge
        if (e.verified) {
            p.setPen(kGreen);
            p.setFont(normalFont);
            p.drawText(QRectF(barX, barY + 12, kBarMaxWidth, 14),
                       Qt::AlignRight | Qt::AlignVCenter, tr("Verified"));
        }

        y += kEntryHeight;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor(120, 120, 120));
        p.setFont(normalFont);
        p.drawText(rect, Qt::AlignCenter, tr("No knowledge entries yet.\nClick \"Extract\" to begin."));
    } else if (entries_.size() > visibleCount) {
        p.setPen(QColor(140, 140, 140));
        p.setFont(normalFont);
        p.drawText(rect.left() + kMargin, rect.bottom() - kMargin,
                   tr("... and %1 more entries").arg(entries_.size() - visibleCount));
    }
}

void PaperKnowledgeExtractor::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Background panel
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 12));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(kPurple);
    p.drawText(rect.adjusted(kMargin, kMargin, 0, 0),
               Qt::AlignLeft | Qt::AlignTop, tr("Categories"));

    if (entries_.isEmpty()) {
        QFont normalFont = font();
        normalFont.setPointSize(normalFont.pointSize() - 1);
        p.setFont(normalFont);
        p.setPen(QColor(120, 120, 120));
        p.drawText(rect.adjusted(0, 40, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   tr("No data"));
        return;
    }

    QMap<QString, int> counts = categoryCounts();
    const QList<QString> keys = counts.keys();
    int total = entries_.size();
    int chartCenterX = rect.left() + kChartSize / 2 + kMargin;
    int chartCenterY = rect.top() + kChartSize / 2 + 30;
    int radius = kChartSize / 2 - 10;

    // Draw pie chart
    qreal startAngle = 0.0;
    const QColor palette[] = {kBlue, kGreen, kAmber, kRed, kPurple};
    for (int i = 0; i < keys.size(); ++i) {
        qreal slice = 360.0 * counts[keys[i]] / static_cast<qreal>(total);

        p.setBrush(palette[i % 5]);
        p.setPen(QColor(30, 30, 30));
        p.drawPie(QRect(chartCenterX - radius, chartCenterY - radius,
                        radius * 2, radius * 2),
                  static_cast<int>(startAngle * 16),
                  static_cast<int>(slice * 16));
        startAngle += slice;
    }

    // Legend
    QFont legendFont = font();
    legendFont.setPointSize(legendFont.pointSize() - 1);
    p.setFont(legendFont);
    int legendY = rect.top() + kMargin + 24;
    for (int i = 0; i < keys.size(); ++i) {
        int lx = rect.right() - 130;
        if (lx < chartCenterX + radius + 10) lx = rect.left() + kMargin;

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i % 5]);
        p.drawRoundedRect(QRect(lx, legendY, 10, 10), 2, 2);

        p.setPen(QColor(200, 200, 200));
        p.drawText(lx + 16, legendY + 10,
                   QStringLiteral("%1 (%2)").arg(keys[i]).arg(counts[keys[i]]));
        legendY += 18;
    }
}

void PaperKnowledgeExtractor::drawStats(QPainter& p, const QRect& rect) {
    // Background panel
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 12));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(kAmber);
    p.drawText(rect.adjusted(kMargin, kMargin, 0, 0),
               Qt::AlignLeft | Qt::AlignTop, tr("Statistics"));

    QFont statFont = font();
    statFont.setPointSize(statFont.pointSize() + 6);
    statFont.setBold(true);

    QFont labelFont = font();
    labelFont.setPointSize(labelFont.pointSize() - 1);

    int y = rect.top() + kMargin + 36;
    int centerX = rect.left() + rect.width() / 2;

    // Total entries
    p.setFont(statFont);
    p.setPen(kBlue);
    p.drawText(QRect(rect.left() + kMargin, y, rect.width() - 2 * kMargin, 40),
               Qt::AlignCenter, QString::number(entries_.size()));
    y += 36;
    p.setFont(labelFont);
    p.setPen(QColor(160, 160, 160));
    p.drawText(QRect(rect.left() + kMargin, y, rect.width() - 2 * kMargin, 20),
               Qt::AlignCenter, tr("Total Entries"));
    y += 32;

    // Divider
    p.setPen(QColor(255, 255, 255, 30));
    p.drawLine(rect.left() + 20, y, rect.right() - 20, y);
    y += 16;

    // Verified count
    int verified = verifiedCount();
    p.setFont(statFont);
    p.setPen(kGreen);
    p.drawText(QRect(rect.left() + kMargin, y, rect.width() - 2 * kMargin, 40),
               Qt::AlignCenter, QString::number(verified));
    y += 36;
    p.setFont(labelFont);
    p.setPen(QColor(160, 160, 160));
    p.drawText(QRect(rect.left() + kMargin, y, rect.width() - 2 * kMargin, 20),
               Qt::AlignCenter, tr("Verified"));
    y += 32;

    // Divider
    p.setPen(QColor(255, 255, 255, 30));
    p.drawLine(rect.left() + 20, y, rect.right() - 20, y);
    y += 16;

    // Average confidence
    qreal avg = avgConfidence();
    p.setFont(statFont);
    QColor confColor = avg >= 0.8 ? kGreen : avg >= 0.6 ? kAmber : kRed;
    p.setPen(confColor);
    p.drawText(QRect(rect.left() + kMargin, y, rect.width() - 2 * kMargin, 40),
               Qt::AlignCenter, QStringLiteral("%1%").arg(avg * 100, 0, 'f', 1));
    y += 36;
    p.setFont(labelFont);
    p.setPen(QColor(160, 160, 160));
    p.drawText(QRect(rect.left() + kMargin, y, rect.width() - 2 * kMargin, 20),
               Qt::AlignCenter, tr("Avg Confidence"));
    y += 32;

    // Divider
    p.setPen(QColor(255, 255, 255, 30));
    p.drawLine(rect.left() + 20, y, rect.right() - 20, y);
    y += 16;

    // Categories count
    int catCount = categoryCounts().size();
    p.setFont(statFont);
    p.setPen(kPurple);
    p.drawText(QRect(rect.left() + kMargin, y, rect.width() - 2 * kMargin, 40),
               Qt::AlignCenter, QString::number(catCount));
    y += 36;
    p.setFont(labelFont);
    p.setPen(QColor(160, 160, 160));
    p.drawText(QRect(rect.left() + kMargin, y, rect.width() - 2 * kMargin, 20),
               Qt::AlignCenter, tr("Categories"));
}

void PaperKnowledgeExtractor::updateInfo() {
    int total = entries_.size();
    int verified = verifiedCount();
    qreal avg = avgConfidence();
    int cats = categoryCounts().size();

    infoLabel_->setText(
        tr("Entries: %1 | Verified: %2 | Avg Confidence: %3% | Categories: %4")
            .arg(total)
            .arg(verified)
            .arg(avg * 100, 0, 'f', 1)
            .arg(cats));
}

void PaperKnowledgeExtractor::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray(QStringLiteral("entries"));
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        KnowEntry e;
        e.id = settings_.value(QStringLiteral("id"), i).toInt();
        e.entity = settings_.value(QStringLiteral("entity")).toString();
        e.category = settings_.value(QStringLiteral("category")).toString();
        e.relation = settings_.value(QStringLiteral("relation")).toString();
        e.confidence = settings_.value(QStringLiteral("confidence"), 0.0).toReal();
        e.frequency = settings_.value(QStringLiteral("frequency"), 1.0).toReal();
        e.sources = settings_.value(QStringLiteral("sources"), 1).toInt();
        e.verified = settings_.value(QStringLiteral("verified"), false).toBool();
        e.color = QColor(settings_.value(QStringLiteral("color"), kBlue.name()).toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
    update();
}

void PaperKnowledgeExtractor::saveSettings() {
    settings_.beginWriteArray(QStringLiteral("entries"),
                              static_cast<int>(entries_.size()));
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const KnowEntry& e = entries_.at(i);
        settings_.setValue(QStringLiteral("id"), e.id);
        settings_.setValue(QStringLiteral("entity"), e.entity);
        settings_.setValue(QStringLiteral("category"), e.category);
        settings_.setValue(QStringLiteral("relation"), e.relation);
        settings_.setValue(QStringLiteral("confidence"), e.confidence);
        settings_.setValue(QStringLiteral("frequency"), e.frequency);
        settings_.setValue(QStringLiteral("sources"), e.sources);
        settings_.setValue(QStringLiteral("verified"), e.verified);
        settings_.setValue(QStringLiteral("color"), e.color.name());
    }
    settings_.endArray();
}
