#include "analysis/PaperEntityLinker.hpp"
#include <QRandomGenerator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPainterPath>
#include <QFontMetrics>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

static const QVector<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};

PaperEntityLinker::PaperEntityLinker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EntityLinker")
    , linkBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperEntityLinker::addEntry(const EntityLinkEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    update();
    saveSettings();
}

QList<EntityLinkEntry> PaperEntityLinker::entries() const
{
    return entries_;
}

int PaperEntityLinker::confirmedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.confirmed) ++count;
    }
    return count;
}

qreal PaperEntityLinker::avgConfidence() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperEntityLinker::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperEntityLinker::onLink()
{
    const QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QRegularExpression re(R"(\s*->\s*)");
    const QStringList parts = text.split(re);
    if (parts.size() < 2) return;

    const QString entity  = parts[0].trimmed();
    const QString linked  = parts[1].trimmed();
    if (entity.isEmpty() || linked.isEmpty()) return;

    const QString category = categoryCombo_->currentText();

    EntityLinkEntry entry;
    entry.id            = static_cast<int>(reinterpret_cast<quintptr>(this)) + entries_.size();
    entry.entity        = entity;
    entry.category      = category;
    entry.linked        = linked;
    entry.confidence    = QRandomGenerator::global()->bounded(50, 100) / 100.0;
    entry.frequency     = QRandomGenerator::global()->bounded(10, 500) / 100.0;
    entry.cooccurrences = QRandomGenerator::global()->bounded(1, 200);
    entry.confirmed     = entry.confidence > 0.8;
    entry.color         = kPalette[entries_.size() % kPalette.size()];

    entries_.append(entry);
    emit entityLinked(entry.id, entry.confidence);

    inputField_->clear();
    updateInfo();
    update();
    saveSettings();
}

void PaperEntityLinker::onClear()
{
    entries_.clear();
    inputField_->clear();
    updateInfo();
    update();
    saveSettings();
}

void PaperEntityLinker::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    // Title bar
    QPainterPath titlePath;
    titlePath.addRoundedRect(0, 0, w, 48, 0, 0);
    p.fillPath(titlePath, QColor("#1e293b"));
    p.setPen(Qt::white);
    QFont titleFont = font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(QRect(16, 0, w - 32, 48), Qt::AlignVCenter | Qt::AlignLeft,
               QStringLiteral("Entity Linker"));

    const int contentTop = 56;
    const int listH      = qMax(200, h - contentTop - 260);
    const int chartTop   = contentTop + listH + 12;
    const int chartH     = qMax(140, (h - chartTop - 100) / 2);
    const int statsTop   = chartTop + chartH + 12;

    drawEntityList(p, QRect(12, contentTop, w - 24, listH));
    drawCategoryChart(p, QRect(12, chartTop, w - 24, chartH));
    drawStats(p, QRect(12, statsTop, w - 24, h - statsTop - 12));
}

void PaperEntityLinker::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 56, 12, 12);
    mainLayout->setSpacing(8);

    // Input row
    auto* inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("entity -> linked"));
    inputField_->setMinimumHeight(36);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({QStringLiteral("Person"), QStringLiteral("Organization"),
                              QStringLiteral("Location"), QStringLiteral("Concept"),
                              QStringLiteral("Method"), QStringLiteral("Dataset")});
    categoryCombo_->setMinimumWidth(130);
    categoryCombo_->setMinimumHeight(36);

    linkBtn_ = new QPushButton(QStringLiteral("Link"), this);
    linkBtn_->setMinimumHeight(36);
    linkBtn_->setMinimumWidth(80);
    linkBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:6px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");

    clearBtn_ = new QPushButton(QStringLiteral("Clear"), this);
    clearBtn_->setMinimumHeight(36);
    clearBtn_->setMinimumWidth(80);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:6px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}"
        "QPushButton:pressed{background:#991b1b;}");

    inputRow->addWidget(inputField_);
    inputRow->addWidget(categoryCombo_);
    inputRow->addWidget(linkBtn_);
    inputRow->addWidget(clearBtn_);

    // Info label
    infoLabel_ = new QLabel(this);
    infoLabel_->setMinimumHeight(28);
    infoLabel_->setStyleSheet("color:#475569;font-size:12px;");

    // Spacer to let paintEvent handle the custom drawing below
    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addLayout(inputRow);
    mainLayout->addWidget(infoLabel_);
    mainLayout->addWidget(spacer);

    connect(linkBtn_, &QPushButton::clicked, this, &PaperEntityLinker::onLink);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEntityLinker::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperEntityLinker::onLink);
}

void PaperEntityLinker::drawEntityList(QPainter& p, const QRect& rect)
{
    // Card background
    QPainterPath card;
    card.addRoundedRect(rect, 10, 10);
    p.fillPath(card, Qt::white);

    QPen borderPen(QColor("#e2e8f0"), 1);
    p.setPen(borderPen);
    p.drawPath(card);

    // Header
    p.setPen(QColor("#334155"));
    QFont hdrFont = font();
    hdrFont.setPointSize(11);
    hdrFont.setBold(true);
    p.setFont(hdrFont);
    p.drawText(rect.adjusted(14, 8, 0, 0), Qt::AlignTop | Qt::AlignLeft,
               QStringLiteral("Linked Entities"));

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont emptyFont = font();
        emptyFont.setPointSize(11);
        p.setFont(emptyFont);
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   QStringLiteral("No entity links yet. Use 'entity -> linked' format above."));
        return;
    }

    // Column headers
    const int rowY0    = rect.y() + 36;
    const int colEntity = rect.x() + 14;
    const int colLinked = rect.x() + rect.width() * 0.30;
    const int colCat    = rect.x() + rect.width() * 0.55;
    const int colConf   = rect.x() + rect.width() * 0.72;
    const int colFreq   = rect.x() + rect.width() * 0.84;
    const int colStatus = rect.x() + rect.width() * 0.94;

    p.setPen(QColor("#64748b"));
    QFont colFont = font();
    colFont.setPointSize(9);
    colFont.setBold(true);
    p.setFont(colFont);
    p.drawText(colEntity, rowY0, QStringLiteral("Entity"));
    p.drawText(colLinked, rowY0, QStringLiteral("Linked"));
    p.drawText(colCat, rowY0, QStringLiteral("Category"));
    p.drawText(colConf, rowY0, QStringLiteral("Conf"));
    p.drawText(colFreq, rowY0, QStringLiteral("Freq"));
    p.drawText(colStatus, rowY0, QStringLiteral("OK"));

    // Separator
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.drawLine(rect.x() + 8, rowY0 + 16, rect.right() - 8, rowY0 + 16);

    // Rows
    QFont dataFont = font();
    dataFont.setPointSize(9);
    dataFont.setBold(false);
    p.setFont(dataFont);

    const int rowHeight = 22;
    int y = rowY0 + 30;
    const int maxVisible = (rect.height() - 66) / rowHeight;

    for (int i = 0; i < qMin(entries_.size(), maxVisible); ++i) {
        const auto& e = entries_[i];

        // Alternating row background
        if (i % 2 == 0) {
            p.fillRect(QRect(rect.x() + 4, y - 12, rect.width() - 8, rowHeight),
                       QColor("#f8fafc"));
        }

        // Color dot
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(colEntity - 2, y - 8, 8, 8);
        p.setBrush(Qt::NoBrush);

        p.setPen(QColor("#1e293b"));
        p.drawText(colEntity + 10, y, e.entity);

        p.setPen(QColor("#475569"));
        p.drawText(colLinked, y, e.linked);

        p.setPen(QColor("#64748b"));
        p.drawText(colCat, y, e.category);

        // Confidence color coded
        if (e.confidence > 0.8)      p.setPen(QColor("#16a34a"));
        else if (e.confidence > 0.5) p.setPen(QColor("#d97706"));
        else                         p.setPen(QColor("#dc2626"));
        p.drawText(colConf, y, QString::number(e.confidence, 'f', 2));

        p.setPen(QColor("#475569"));
        p.drawText(colFreq, y, QString::number(e.frequency, 'f', 1));

        p.drawText(colStatus, y, e.confirmed ? QStringLiteral("Y") : QStringLiteral("N"));

        y += rowHeight;
    }
}

void PaperEntityLinker::drawCategoryChart(QPainter& p, const QRect& rect)
{
    QPainterPath card;
    card.addRoundedRect(rect, 10, 10);
    p.fillPath(card, Qt::white);

    QPen borderPen(QColor("#e2e8f0"), 1);
    p.setPen(borderPen);
    p.drawPath(card);

    QFont hdrFont = font();
    hdrFont.setPointSize(11);
    hdrFont.setBold(true);
    p.setPen(QColor("#334155"));
    p.setFont(hdrFont);
    p.drawText(rect.adjusted(14, 8, 0, 0), Qt::AlignTop | Qt::AlignLeft,
               QStringLiteral("Category Distribution"));

    if (entries_.isEmpty()) return;

    const auto counts = categoryCounts();
    int maxCount = 0;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it) {
        if (it.value() > maxCount) maxCount = it.value();
    }
    if (maxCount == 0) maxCount = 1;

    const int barAreaLeft   = rect.x() + 80;
    const int barAreaWidth  = rect.width() - 120;
    const int barAreaTop    = rect.y() + 32;
    const int barAreaHeight = rect.height() - 48;
    const int barCount      = counts.size();
    const int barSpacing    = 6;
    const int barHeight     = qMax(8, (barAreaHeight - barSpacing * (barCount - 1)) / barCount);

    QFont labelFont = font();
    labelFont.setPointSize(9);
    p.setFont(labelFont);

    int idx = 0;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it, ++idx) {
        const int y = barAreaTop + idx * (barHeight + barSpacing);
        const int bw = static_cast<int>(
            (static_cast<qreal>(it.value()) / maxCount) * barAreaWidth);

        // Category label
        p.setPen(QColor("#475569"));
        p.drawText(QRect(rect.x() + 8, y - 2, 70, barHeight + 4),
                   Qt::AlignRight | Qt::AlignVCenter, it.key());

        // Bar
        const QColor barColor = kPalette[idx % kPalette.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        QPainterPath barPath;
        barPath.addRoundedRect(barAreaLeft, y, bw, barHeight, 4, 4);
        p.drawPath(barPath);

        // Count label
        p.setPen(QColor("#334155"));
        p.drawText(barAreaLeft + bw + 8, y + barHeight - 3,
                   QString::number(it.value()));
    }

    p.setBrush(Qt::NoBrush);
}

void PaperEntityLinker::drawStats(QPainter& p, const QRect& rect)
{
    QPainterPath card;
    card.addRoundedRect(rect, 10, 10);
    p.fillPath(card, Qt::white);

    QPen borderPen(QColor("#e2e8f0"), 1);
    p.setPen(borderPen);
    p.drawPath(card);

    QFont hdrFont = font();
    hdrFont.setPointSize(11);
    hdrFont.setBold(true);
    p.setPen(QColor("#334155"));
    p.setFont(hdrFont);
    p.drawText(rect.adjusted(14, 8, 0, 0), Qt::AlignTop | Qt::AlignLeft,
               QStringLiteral("Statistics"));

    const int statY = rect.y() + 30;
    QFont statFont = font();
    statFont.setPointSize(10);
    p.setFont(statFont);

    const int col1 = rect.x() + 14;
    const int col2 = rect.x() + rect.width() / 3 + 14;
    const int col3 = rect.x() + 2 * rect.width() / 3 + 14;
    const int rowGap = 20;

    // Stat entries
    p.setPen(QColor("#64748b"));
    p.drawText(col1, statY, QStringLiteral("Total:"));
    p.drawText(col2, statY, QStringLiteral("Confirmed:"));
    p.drawText(col3, statY, QStringLiteral("Avg Confidence:"));

    p.setPen(QColor("#1e293b"));
    QFont valFont = statFont;
    valFont.setBold(true);
    p.setFont(valFont);
    p.drawText(col1, statY + rowGap, QString::number(entries_.size()));
    p.drawText(col2, statY + rowGap, QString::number(confirmedCount()));
    p.drawText(col3, statY + rowGap, QString::number(avgConfidence(), 'f', 3));

    // Confidence meter bar
    const int meterY    = statY + rowGap * 2 + 8;
    const int meterW    = rect.width() - 28;
    const int meterH    = 10;
    const qreal avgConf = avgConfidence();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#e2e8f0"));
    p.drawRoundedRect(col1, meterY, meterW, meterH, 5, 5);

    QColor meterColor = avgConf > 0.8 ? QColor("#16a34a")
                      : avgConf > 0.5 ? QColor("#d97706")
                      :                  QColor("#dc2626");
    p.setBrush(meterColor);
    p.drawRoundedRect(col1, meterY,
                      static_cast<int>(meterW * avgConf), meterH, 5, 5);
    p.setBrush(Qt::NoBrush);

    // Cooccurrences summary
    if (!entries_.isEmpty()) {
        int totalCo = 0;
        for (const auto& e : entries_) totalCo += e.cooccurrences;
        p.setFont(statFont);
        p.setPen(QColor("#64748b"));
        p.drawText(col1, meterY + meterH + 18,
                   QStringLiteral("Total Co-occurrences:"));
        p.setPen(QColor("#1e293b"));
        p.setFont(valFont);
        p.drawText(col1 + 150, meterY + meterH + 18,
                   QString::number(totalCo));
    }
}

void PaperEntityLinker::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(QStringLiteral("No entries."));
        return;
    }

    const auto cCounts = categoryCounts();
    QString topCat;
    int topCount = 0;
    for (auto it = cCounts.cbegin(); it != cCounts.cend(); ++it) {
        if (it.value() > topCount) {
            topCount = it.value();
            topCat   = it.key();
        }
    }

    infoLabel_->setText(
        QStringLiteral("Entries: %1  |  Confirmed: %2  |  "
                       "Avg Confidence: %3  |  Top Category: %4 (%5)")
            .arg(entries_.size())
            .arg(confirmedCount())
            .arg(QString::number(avgConfidence(), 'f', 2))
            .arg(topCat)
            .arg(topCount));
}

void PaperEntityLinker::loadSettings()
{
    const int size = settings_.beginReadArray(QStringLiteral("entries"));
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EntityLinkEntry e;
        e.id            = settings_.value(QStringLiteral("id")).toInt();
        e.entity        = settings_.value(QStringLiteral("entity")).toString();
        e.category      = settings_.value(QStringLiteral("category")).toString();
        e.linked        = settings_.value(QStringLiteral("linked")).toString();
        e.confidence    = settings_.value(QStringLiteral("confidence")).toReal();
        e.frequency     = settings_.value(QStringLiteral("frequency")).toReal();
        e.cooccurrences = settings_.value(QStringLiteral("cooccurrences")).toInt();
        e.confirmed     = settings_.value(QStringLiteral("confirmed")).toBool();
        e.color         = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEntityLinker::saveSettings()
{
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue(QStringLiteral("id"),            e.id);
        settings_.setValue(QStringLiteral("entity"),        e.entity);
        settings_.setValue(QStringLiteral("category"),      e.category);
        settings_.setValue(QStringLiteral("linked"),        e.linked);
        settings_.setValue(QStringLiteral("confidence"),    e.confidence);
        settings_.setValue(QStringLiteral("frequency"),     e.frequency);
        settings_.setValue(QStringLiteral("cooccurrences"), e.cooccurrences);
        settings_.setValue(QStringLiteral("confirmed"),     e.confirmed);
        settings_.setValue(QStringLiteral("color"),         e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
