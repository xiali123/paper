#include "reading/PaperReadingVoyage2.hpp"
#include <QVBoxLayout>
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

static const QStringList kDestinations = {
    "Knowledge Island", "Theory Archipelago", "Method Atoll"
};

static const QStringList kVessels = {
    "Paper Boat", "Research Vessel", "Citation Ship"
};

static const QStringList kCategories = {
    "Literature", "Science", "Engineering", "Medicine", "Arts"
};

PaperReadingVoyage2::PaperReadingVoyage2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingVoyage2")
    , sailBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingVoyage2::setupUI()
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
    inputField_->setPlaceholderText(tr("Enter destination..."));
    inputField_->setMinimumWidth(200);
    toolbar->addWidget(inputField_);

    sailBtn_ = new QPushButton(tr("Sail"), this);
    sailBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    toolbar->addWidget(sailBtn_);

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

    connect(sailBtn_, &QPushButton::clicked, this, &PaperReadingVoyage2::onSail);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingVoyage2::onClear);
}

void PaperReadingVoyage2::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    p.fillRect(rect(), QColor("#f8fafc"));

    const int toolbarH = (sailBtn_ ? sailBtn_->geometry().bottom() : 0) + 16;
    const int drawTop = qMax(toolbarH, 60);

    // Left 60%: voyage view
    const QRect voyageRect(10, drawTop, static_cast<int>(w * 0.6) - 15, h - drawTop - 10);
    // Right 40% top: category donut chart
    const QRect chartRect(static_cast<int>(w * 0.6) + 5, drawTop,
                          static_cast<int>(w * 0.4) - 15,
                          static_cast<int>((h - drawTop - 10) * 0.75));
    // Right 40% bottom: stats boxes
    const int statsTop = drawTop + static_cast<int>((h - drawTop - 10) * 0.75) + 5;
    const QRect statsRect(static_cast<int>(w * 0.6) + 5, statsTop,
                          static_cast<int>(w * 0.4) - 15,
                          h - statsTop - 10);

    drawVoyageView(p, voyageRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingVoyage2::drawVoyageView(QPainter& p, const QRect& rect)
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
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Voyage Map"));

    if (entries_.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No voyages yet. Click Sail to begin."));
        return;
    }

    QFont normal = p.font();
    normal.setBold(false);
    normal.setPointSize(9);
    p.setFont(normal);

    const int leftMargin = 16;
    const int topMargin = 40;
    const int cardH = 64;
    const int gap = 8;
    const int pathW = rect.width() - leftMargin * 2;

    int y = rect.y() + topMargin;
    const int maxVisible = qMax(1, (rect.height() - topMargin - 10) / (cardH + gap));
    const int startIdx = qMax(0, entries_.size() - maxVisible);

    // Compute max distance for bar scaling
    const qreal maxDist = [&]() {
        qreal m = 0.0;
        for (const auto& e : entries_)
            m = qMax(m, e.distance);
        return m > 0.0 ? m : 1.0;
    }();

    for (int i = startIdx; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);

        // Card background with subtle color tint
        QColor cardBg(e.color.red(), e.color.green(), e.color.blue(), 18);
        p.setPen(Qt::NoPen);
        p.setBrush(cardBg);
        p.drawRoundedRect(rect.x() + leftMargin - 4, y - 2, pathW + 8, cardH, 6, 6);

        // Destination name
        p.setPen(QColor("#1e293b"));
        QFont titleFont = p.font();
        titleFont.setBold(true);
        titleFont.setPointSize(9);
        p.setFont(titleFont);
        p.drawText(rect.x() + leftMargin, y + 13, e.destination);

        // Vessel badge (right side, top line)
        QFont badgeFont = p.font();
        badgeFont.setBold(false);
        badgeFont.setPointSize(8);
        p.setFont(badgeFont);

        const int badgeW = p.fontMetrics().horizontalAdvance(e.vessel) + 12;
        const int badgeX = rect.x() + leftMargin + pathW - badgeW;
        const int badgeY = y + 2;
        const int badgeH = 16;

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(badgeX, badgeY, badgeW, badgeH, 3, 3);
        p.setPen(QColor("#ffffff"));
        p.drawText(QRect(badgeX, badgeY, badgeW, badgeH), Qt::AlignCenter, e.vessel);

        // Ship-path: distance bar
        normal.setBold(false);
        normal.setPointSize(9);
        p.setFont(normal);

        const int trailY = y + 26;
        const int trailH = 14;
        const int barMaxW = pathW - 50;
        const int barW = static_cast<int>(barMaxW * e.distance / maxDist);

        // Background track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(rect.x() + leftMargin, trailY, barMaxW, trailH, 3, 3);

        // Filled distance bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + leftMargin, trailY, barW, trailH, 3, 3);

        // Distance label beside bar
        p.setPen(QColor("#374151"));
        p.drawText(rect.x() + leftMargin + barMaxW + 6, trailY + trailH - 2,
                   QStringLiteral("%1 nm").arg(e.distance, 0, 'f', 1));

        // Port markers along the bar
        const int numPorts = e.ports > 0 ? e.ports : 1;
        for (int pi = 0; pi < numPorts; ++pi) {
            const qreal frac = static_cast<qreal>(pi + 1) / (numPorts + 1);
            const int dotX = rect.x() + leftMargin + static_cast<int>(frac * barW) - 3;
            const int dotY = trailY + trailH / 2 - 3;

            p.setPen(Qt::NoPen);
            p.setBrush(Qt::white);
            p.drawEllipse(dotX, dotY, 6, 6);

            p.setBrush(e.color);
            p.drawEllipse(dotX + 1, dotY + 1, 4, 4);
        }

        // Arrived anchor icon (bottom-right of card)
        if (e.arrived) {
            QFont anchorFont = p.font();
            anchorFont.setBold(true);
            anchorFont.setPointSize(11);
            p.setFont(anchorFont);
            p.setPen(QColor("#16a34a"));
            p.drawText(rect.x() + leftMargin + pathW - 18, y + cardH - 6, tr("\342\x9a\x93"));
        }

        y += cardH + gap;
    }
}

void PaperReadingVoyage2::drawCategoryChart(QPainter& p, const QRect& rect)
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

        // Pie slice
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

void PaperReadingVoyage2::drawStats(QPainter& p, const QRect& rect)
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

    // Calculate total ports
    int totalPorts = 0;
    for (const auto& e : entries_)
        totalPorts += e.ports;

    // 4 stat boxes
    const int boxW = (rect.width() - 50) / 4;
    const int boxH = 42;
    const int bx = rect.x() + 10;
    const int by = rect.y() + 32;

    struct StatItem { QString label; QString value; QColor accent; };
    StatItem items[] = {
        {tr("Voyages"), QString::number(entries_.size()), kPalette[0]},
        {tr("Arrived"), QString::number(arrivedCount()), kPalette[1]},
        {tr("Avg Dist"), QString::number(avgDistance(), 'f', 1) + " nm", kPalette[2]},
        {tr("Ports"), QString::number(totalPorts), kPalette[3]}
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

void PaperReadingVoyage2::onSail()
{
    auto* rng = QRandomGenerator::global();

    ReadingVoyage2Entry entry;
    entry.id = entries_.size() + 1;

    // Destination: from input or random
    QString searchText = inputField_->text().trimmed();
    if (!searchText.isEmpty()) {
        entry.destination = searchText;
    } else {
        entry.destination = kDestinations.at(rng->bounded(kDestinations.size()));
    }

    // Category: from combo or random if "All"
    int comboIdx = categoryCombo_->currentIndex();
    if (comboIdx == 0) {
        entry.category = kCategories.at(rng->bounded(kCategories.size()));
    } else {
        entry.category = kCategories.at(comboIdx - 1);
    }

    entry.vessel = kVessels.at(rng->bounded(kVessels.size()));
    entry.distance = rng->bounded(10, 501) / 10.0;
    entry.ports = rng->bounded(1, 11);
    entry.arrived = rng->bounded(0, 2) == 1;
    entry.color = kPalette.at(rng->bounded(kPalette.size()));

    entries_.append(entry);
    if (entry.arrived)
        emit portReached(entry.id, entry.distance);

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingVoyage2::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingVoyage2::addEntry(const ReadingVoyage2Entry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<ReadingVoyage2Entry> PaperReadingVoyage2::entries() const
{
    return entries_;
}

int PaperReadingVoyage2::arrivedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.arrived)
            ++count;
    }
    return count;
}

qreal PaperReadingVoyage2::avgDistance() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.distance;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingVoyage2::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingVoyage2::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No voyages recorded."));
        return;
    }

    int totalPorts = 0;
    for (const auto& e : entries_)
        totalPorts += e.ports;

    infoLabel_->setText(tr("Voyages: %1 | Arrived: %2 | Avg dist: %3 nm | Ports: %4")
        .arg(entries_.size())
        .arg(arrivedCount())
        .arg(avgDistance(), 0, 'f', 1)
        .arg(totalPorts));
}

void PaperReadingVoyage2::loadSettings()
{
    settings_.beginGroup("entries");
    const int size = settings_.beginReadArray("list");
    entries_.clear();
    entries_.reserve(size);

    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingVoyage2Entry e;
        e.id          = settings_.value("id").toInt();
        e.destination = settings_.value("destination").toString();
        e.category    = settings_.value("category").toString();
        e.vessel      = settings_.value("vessel").toString();
        e.distance    = settings_.value("distance").toReal();
        e.ports       = settings_.value("ports").toInt();
        e.arrived     = settings_.value("arrived").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    // Seed 8 demo entries if empty
    if (entries_.isEmpty()) {
        auto* rng = QRandomGenerator::global();

        for (int i = 0; i < 8; ++i) {
            ReadingVoyage2Entry e;
            e.id = i + 1;
            e.destination = kDestinations.at(i % kDestinations.size());
            e.category = kCategories.at(i % kCategories.size());
            e.vessel = kVessels.at(i % kVessels.size());
            e.distance = rng->bounded(10, 501) / 10.0;
            e.ports = rng->bounded(1, 11);
            e.arrived = (i % 3 == 0);
            e.color = kPalette.at(i % kPalette.size());
            entries_.append(e);
        }

        saveSettings();
    }

    updateInfo();
    update();
}

void PaperReadingVoyage2::saveSettings()
{
    settings_.beginGroup("entries");
    settings_.beginWriteArray("list", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",          e.id);
        settings_.setValue("destination", e.destination);
        settings_.setValue("category",    e.category);
        settings_.setValue("vessel",      e.vessel);
        settings_.setValue("distance",    e.distance);
        settings_.setValue("ports",       e.ports);
        settings_.setValue("arrived",     e.arrived);
        settings_.setValue("color",       e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
