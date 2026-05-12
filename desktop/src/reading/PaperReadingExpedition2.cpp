#include "reading/PaperReadingExpedition2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QFont>
#include <QtMath>

static const QVector<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};

static const QStringList kTerrains = {
    "Mountains", "Valleys", "Rivers", "Caves", "Forests"
};

static const QStringList kCategories = {
    "Biology", "Physics", "Chemistry", "Math", "CS"
};

PaperReadingExpedition2::PaperReadingExpedition2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingExpedition2")
    , startBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingExpedition2::setupUI()
{
    auto* toolbar = new QHBoxLayout(this);
    toolbar->setContentsMargins(12, 8, 12, 8);
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("All"));
    for (const auto& cat : kCategories)
        categoryCombo_->addItem(cat);
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Search expeditions..."));
    inputField_->setMinimumWidth(200);
    toolbar->addWidget(inputField_);

    startBtn_ = new QPushButton(tr("Start"), this);
    startBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    toolbar->addWidget(startBtn_);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}"
        "QPushButton:pressed{background:#991b1b;}");
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch(1);

    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet("font-size:13px; color:#6b7280;");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    toolbar->addWidget(infoLabel_);

    setMinimumSize(720, 520);

    connect(startBtn_, &QPushButton::clicked, this, &PaperReadingExpedition2::onStart);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingExpedition2::onClear);
}

void PaperReadingExpedition2::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    p.fillRect(rect(), QColor("#f8fafc"));

    const int toolbarH = (startBtn_ ? startBtn_->geometry().bottom() : 0) + 16;
    const int drawTop = qMax(toolbarH, 60);

    // Left 60%: expedition view
    const QRect expeditionRect(10, drawTop, static_cast<int>(w * 0.6) - 15, h - drawTop - 10);
    // Right 40% top: category chart
    const QRect chartRect(static_cast<int>(w * 0.6) + 5, drawTop,
                          static_cast<int>(w * 0.4) - 15,
                          static_cast<int>((h - drawTop - 10) * 0.75));
    // Bottom 25%: stats
    const int statsTop = drawTop + static_cast<int>((h - drawTop - 10) * 0.75) + 5;
    const QRect statsRect(static_cast<int>(w * 0.6) + 5, statsTop,
                          static_cast<int>(w * 0.4) - 15,
                          h - statsTop - 10);

    drawExpeditionView(p, expeditionRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingExpedition2::drawExpeditionView(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Expedition Map"));

    if (entries_.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No expeditions yet. Click Start to begin."));
        return;
    }

    QFont normal = p.font();
    normal.setBold(false);
    normal.setPointSize(9);
    p.setFont(normal);

    const int leftMargin = 16;
    const int topMargin = 40;
    const int cardH = 56;
    const int gap = 8;
    const int pathW = rect.width() - leftMargin * 2;

    int y = rect.y() + topMargin;
    const int maxVisible = qMax(1, (rect.height() - topMargin - 10) / (cardH + gap));
    const int startIdx = qMax(0, entries_.size() - maxVisible);

    // Terrain icon map
    static const QMap<QString, QString> terrainIcons = {
        {"Mountains", "⛰"}, {"Valleys", "⬇"}, {"Rivers", "≈"},
        {"Caves", "◎"}, {"Forests", "⬆"}
    };

    for (int i = startIdx; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);

        // Card background for each entry
        QColor cardBg(e.color.red(), e.color.green(), e.color.blue(), 20);
        p.setPen(Qt::NoPen);
        p.setBrush(cardBg);
        p.drawRoundedRect(rect.x() + leftMargin - 4, y - 2, pathW + 8, cardH, 6, 6);

        // Terrain icon + title
        p.setPen(QColor("#1e293b"));
        QFont titleFont = p.font();
        titleFont.setBold(true);
        titleFont.setPointSize(9);
        p.setFont(titleFont);
        QString icon = terrainIcons.value(e.terrain, "•");
        p.drawText(rect.x() + leftMargin, y + 13,
                   QStringLiteral("%1 %2  [%3]").arg(icon, e.title, e.terrain));

        // Progress trail: winding path
        normal.setBold(false);
        normal.setPointSize(9);
        p.setFont(normal);

        const int trailY = y + 28;
        const int trailH = 16;
        const int numSegments = e.milestones > 0 ? e.milestones : 1;
        const qreal segW = static_cast<qreal>(pathW) / numSegments;

        for (int s = 0; s < numSegments; ++s) {
            const int sx = rect.x() + leftMargin + static_cast<int>(s * segW);
            const int sw = static_cast<int>(segW);
            const qreal segProgress = static_cast<qreal>(s + 1) / numSegments;

            // Winding path segment (alternating up/down)
            int waveOffset = (s % 2 == 0) ? 0 : 4;

            if (segProgress <= e.progress) {
                // Completed segment
                p.setPen(Qt::NoPen);
                p.setBrush(e.color);
            } else {
                // Incomplete segment
                QColor faded(e.color.red(), e.color.green(), e.color.blue(), 60);
                p.setPen(Qt::NoPen);
                p.setBrush(faded);
            }

            // Draw trail segment
            p.drawRoundedRect(sx, trailY + waveOffset, sw - 2, trailH - 4, 2, 2);

            // Milestone dot
            if (sw > 6) {
                p.setPen(Qt::NoPen);
                p.setBrush(Qt::white);
                int dotR = 3;
                int dotX = sx + sw / 2 - dotR;
                int dotY = trailY + waveOffset + (trailH - 4) / 2 - dotR;
                p.drawEllipse(dotX, dotY, dotR * 2, dotR * 2);

                if (segProgress <= e.progress) {
                    p.setBrush(e.color);
                    p.drawEllipse(dotX + 1, dotY + 1, (dotR - 1) * 2, (dotR - 1) * 2);
                }
            }
        }

        // Completed flag
        if (e.completed) {
            QFont flagFont = p.font();
            flagFont.setBold(true);
            flagFont.setPointSize(10);
            p.setFont(flagFont);
            p.setPen(QColor("#16a34a"));
            p.drawText(rect.x() + leftMargin + pathW - 40, y + 13, tr("[Done]"));
        } else {
            // Progress percentage
            QFont pctFont = p.font();
            pctFont.setPointSize(8);
            p.setFont(pctFont);
            p.setPen(QColor("#64748b"));
            p.drawText(rect.x() + leftMargin + pathW - 40, y + 13,
                       QStringLiteral("%1%").arg(static_cast<int>(e.progress * 100)));
        }

        y += cardH + gap;
    }
}

void PaperReadingExpedition2::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Category Distribution"));

    const auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(9);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 30, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("No data yet"));
        return;
    }

    // Donut chart
    const int cx = rect.x() + rect.width() / 2;
    const int cy = rect.y() + rect.height() / 2 + 10;
    const int outerR = qMin(rect.width(), rect.height()) / 2 - 30;
    const int innerR = outerR * 2 / 3;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    qreal angle = 0.0;
    int colorIdx = 0;

    QFont labelFont = p.font();
    labelFont.setBold(false);
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        const qreal span = 360.0 * it.value() / static_cast<qreal>(total);
        const QColor c = kPalette.at(colorIdx % kPalette.size());

        // Draw pie slice
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(angle * 16), static_cast<int>(span * 16));

        // Hollow center for donut
        p.setBrush(QColor("#ffffff"));
        p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        // Label outside the donut
        const qreal midAngle = qDegreesToRadians(angle + span / 2.0);
        const int labelR = outerR + 14;
        const int lx = cx + static_cast<int>(labelR * qCos(midAngle));
        const int ly = cy - static_cast<int>(labelR * qSin(midAngle));

        p.setPen(QColor("#374151"));
        p.drawText(QRect(lx - 50, ly - 8, 100, 16), Qt::AlignCenter,
                   QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()));

        angle += span;
        ++colorIdx;
    }

    // Center text
    QFont centerFont = p.font();
    centerFont.setBold(true);
    centerFont.setPointSize(12);
    p.setFont(centerFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(QRect(cx - 30, cy - 12, 60, 24), Qt::AlignCenter, QString::number(total));
}

void PaperReadingExpedition2::drawStats(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Statistics"));

    QFont body = p.font();
    body.setBold(false);
    body.setPointSize(10);
    p.setFont(body);

    // Calculate total milestones
    int totalMilestones = 0;
    for (const auto& e : entries_)
        totalMilestones += e.milestones;

    // 4 stat boxes
    const int boxW = (rect.width() - 50) / 4;
    const int boxH = 42;
    const int bx = rect.x() + 10;
    const int by = rect.y() + 32;

    struct StatItem { QString label; QString value; QColor accent; };
    StatItem items[] = {
        {tr("Total"), QString::number(entries_.size()), kPalette[0]},
        {tr("Completed"), QString::number(completedCount()), kPalette[1]},
        {tr("Avg Progress"), QString::number(avgProgress() * 100, 'f', 1) + "%", kPalette[2]},
        {tr("Milestones"), QString::number(totalMilestones), kPalette[3]}
    };

    for (int i = 0; i < 4; ++i) {
        const int x = bx + i * (boxW + 10);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f1f5f9"));
        p.drawRoundedRect(x, by, boxW, boxH, 4, 4);

        // Accent line on top
        p.setBrush(items[i].accent);
        p.drawRoundedRect(x, by, boxW, 3, 1, 1);

        // Value
        QFont valFont = p.font();
        valFont.setBold(true);
        valFont.setPointSize(12);
        p.setFont(valFont);
        p.setPen(QColor("#0f172a"));
        p.drawText(QRect(x, by + 4, boxW, 20), Qt::AlignCenter, items[i].value);

        // Label
        body.setBold(false);
        body.setPointSize(8);
        p.setFont(body);
        p.setPen(QColor("#64748b"));
        p.drawText(QRect(x, by + 24, boxW, 16), Qt::AlignCenter, items[i].label);
    }
}

void PaperReadingExpedition2::onStart()
{
    auto* rng = QRandomGenerator::global();

    ReadingExpedition2Entry entry;
    entry.id = entries_.size() + 1;

    // Use search text for title, or generate random one
    QString searchText = inputField_->text().trimmed();
    if (!searchText.isEmpty()) {
        entry.title = searchText;
    } else {
        const QString cat = kCategories.at(rng->bounded(kCategories.size()));
        const QString terrain = kTerrains.at(rng->bounded(kTerrains.size()));
        entry.title = QStringLiteral("%1-%2 %3").arg(cat, terrain).arg(entry.id);
    }

    // Category: from combo or random if "All"
    int comboIdx = categoryCombo_->currentIndex();
    if (comboIdx == 0) {
        entry.category = kCategories.at(rng->bounded(kCategories.size()));
    } else {
        entry.category = kCategories.at(comboIdx - 1);
    }

    entry.terrain = kTerrains.at(rng->bounded(kTerrains.size()));
    entry.progress = rng->bounded(0, 101) / 100.0;
    entry.milestones = rng->bounded(3, 12);
    entry.completed = entry.progress >= 1.0;
    entry.color = kPalette.at(rng->bounded(kPalette.size()));

    entries_.append(entry);
    if (entry.completed)
        emit expeditionCompleted(entry.id, entry.progress);

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingExpedition2::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingExpedition2::addEntry(const ReadingExpedition2Entry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<ReadingExpedition2Entry> PaperReadingExpedition2::entries() const
{
    return entries_;
}

int PaperReadingExpedition2::completedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.completed)
            ++count;
    }
    return count;
}

qreal PaperReadingExpedition2::avgProgress() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.progress;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingExpedition2::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingExpedition2::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No expeditions recorded."));
        return;
    }

    int totalMilestones = 0;
    for (const auto& e : entries_)
        totalMilestones += e.milestones;

    infoLabel_->setText(tr("Expeditions: %1 | Completed: %2 | Avg: %3% | Milestones: %4")
        .arg(entries_.size())
        .arg(completedCount())
        .arg(avgProgress() * 100, 0, 'f', 1)
        .arg(totalMilestones));
}

void PaperReadingExpedition2::loadSettings()
{
    settings_.beginGroup("entries");
    const int size = settings_.beginReadArray("list");
    entries_.clear();
    entries_.reserve(size);

    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingExpedition2Entry e;
        e.id         = settings_.value("id").toInt();
        e.title      = settings_.value("title").toString();
        e.category   = settings_.value("category").toString();
        e.terrain    = settings_.value("terrain").toString();
        e.progress   = settings_.value("progress").toReal();
        e.milestones = settings_.value("milestones").toInt();
        e.completed  = settings_.value("completed").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    // Seed 8 demo entries if empty
    if (entries_.isEmpty()) {
        auto* rng = QRandomGenerator::global();
        const QStringList demoTitles = {
            "Alpine Genome Study", "Quantum Valley Survey", "Riverbed Chemistry",
            "Deep Cave Mathematics", "Forest Algorithm Research", "Mountain Protein Analysis",
            "Valley Optics Experiment", "River Topology Proof"
        };

        for (int i = 0; i < 8; ++i) {
            ReadingExpedition2Entry e;
            e.id = i + 1;
            e.title = demoTitles.at(i);
            e.category = kCategories.at(i % kCategories.size());
            e.terrain = kTerrains.at(i % kTerrains.size());
            e.progress = rng->bounded(10, 101) / 100.0;
            e.milestones = rng->bounded(3, 12);
            e.completed = e.progress >= 1.0;
            e.color = kPalette.at(i % kPalette.size());
            entries_.append(e);
        }

        saveSettings();
    }

    updateInfo();
    update();
}

void PaperReadingExpedition2::saveSettings()
{
    settings_.beginGroup("entries");
    settings_.beginWriteArray("list", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",         e.id);
        settings_.setValue("title",      e.title);
        settings_.setValue("category",   e.category);
        settings_.setValue("terrain",    e.terrain);
        settings_.setValue("progress",   e.progress);
        settings_.setValue("milestones", e.milestones);
        settings_.setValue("completed",  e.completed);
        settings_.setValue("color",      e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
