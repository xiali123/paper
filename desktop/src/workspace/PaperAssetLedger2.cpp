#include "workspace/PaperAssetLedger2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperAssetLedger2::PaperAssetLedger2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AssetLedger2")
{
    setupUI();
    loadSettings();
}

void PaperAssetLedger2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Equipment", "Software", "Data", "IP", "Facility"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 100px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter asset name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    tradeBtn_ = new QPushButton("Trade");
    tradeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(tradeBtn_, &QPushButton::clicked, this, &PaperAssetLedger2::onTrade);
    toolbar->addWidget(tradeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 4px 12px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAssetLedger2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Assets: 0 | Active: 0 | Total Value: $0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 2px 4px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(720, 560);
}

void PaperAssetLedger2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    int w = width(), h = height();
    int toolbarH = 80;

    // Top half: ledger view
    drawLedgerView(p, QRect(10, toolbarH, w - 20, (h - toolbarH) / 2 - 10));

    // Bottom-left: category chart
    drawCategoryChart(p, QRect(10, (h + toolbarH) / 2 + 5, w / 2 - 15, h / 2 - toolbarH / 2 - 20));

    // Bottom-right: stats
    drawStats(p, QRect(w / 2 + 5, (h + toolbarH) / 2 + 5, w / 2 - 15, h / 2 - toolbarH / 2 - 20));
}

void PaperAssetLedger2::drawLedgerView(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 4, "Asset Ledger");

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 11));
        p.drawText(rect, Qt::AlignCenter, "No assets recorded");
        return;
    }

    // Column header
    int headerY = rect.y() + 4;
    int rowH = 34;
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + 4, headerY, 100, 16, Qt::AlignVCenter, "Asset");
    p.drawText(rect.x() + 110, headerY, 70, 16, Qt::AlignVCenter, "Category");
    p.drawText(rect.x() + 190, headerY, 40, 16, Qt::AlignVCenter, "Value");
    p.drawText(rect.x() + 240, headerY, 60, 16, Qt::AlignVCenter, "Txns");
    p.drawText(rect.x() + rect.width() - 60, headerY, 50, 16,
               Qt::AlignVCenter | Qt::AlignRight, "Status");

    // Separator
    p.setPen(QColor(226, 232, 240));
    p.drawLine(rect.x(), headerY + 16, rect.x() + rect.width(), headerY + 16);

    int maxShow = qMin(static_cast<int>(entries_.size()),
                       (rect.height() - 26) / (rowH + 3));
    qreal maxVal = 1.0;
    for (int i = 0; i < maxShow; ++i)
        maxVal = qMax(maxVal, entries_[i].value);

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = entries_[i];
        int y = headerY + 20 + i * (rowH + 3);

        // Background bar proportional to value
        int barW = static_cast<int>((e.value / maxVal) * (rect.width() - 4));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        QPainterPath bgPath;
        bgPath.addRoundedRect(rect.x(), y, barW, rowH, 4, 4);
        p.drawPath(bgPath);

        // Color indicator stripe on the left
        p.setBrush(e.color);
        QPainterPath stripePath;
        stripePath.addRoundedRect(rect.x(), y, 4, rowH, 2, 2);
        p.drawPath(stripePath);

        // Asset name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y, 95, rowH, Qt::AlignVCenter,
                   e.asset.left(14));

        // Category
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 110, y, 70, rowH, Qt::AlignVCenter,
                   e.category);

        // Value
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 190, y, 40, rowH, Qt::AlignVCenter,
                   "$" + QString::number(e.value, 'f', 0));

        // Transaction count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 240, y, 60, rowH, Qt::AlignVCenter,
                   QString::number(e.transactions) + " txns");

        // Status badge
        int badgeX = rect.x() + rect.width() - 72;
        int badgeY = y + 8;
        int badgeW = 64;
        int badgeH = 18;

        // Badge background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 9, 9);
        p.drawPath(badgePath);

        // Badge text
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, badgeH,
                   Qt::AlignCenter, e.status);

        // Active indicator dot
        int dotX = rect.x() + rect.width() - 10;
        int dotY = y + rowH / 2;
        p.setPen(Qt::NoPen);
        if (e.active) {
            p.setBrush(QColor(0x16, 0xa3, 0x4a)); // green
        } else {
            p.setBrush(QColor(0xcb, 0xd5, 0xe1)); // grey
        }
        p.drawEllipse(dotX - 4, dotY - 4, 8, 8);
    }
}

void PaperAssetLedger2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Breakdown");

    auto counts = categoryCounts();
    struct CatInfo { QString key; QString label; QColor color; };
    CatInfo cats[] = {
        {"Equipment", "Equip",  QColor(0x3b, 0x82, 0xf6)},
        {"Software",  "Soft",   QColor(0x16, 0xa3, 0x4a)},
        {"Data",      "Data",   QColor(0xd9, 0x77, 0x06)},
        {"IP",        "IP",     QColor(0xdc, 0x26, 0x26)},
        {"Facility",  "Facil",  QColor(0x7c, 0x3a, 0xed)},
    };

    int maxCount = 1;
    for (const auto& c : cats) {
        int v = counts.contains(c.key) ? counts[c.key] : 0;
        maxCount = qMax(maxCount, v);
    }

    int barH = qMin(24, (rect.height() - 30) / 5);
    int chartW = rect.width() - 80;

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i].key) ? counts[cats[i].key] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxCount) * chartW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 36, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i].label);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(cats[i].color);
        QPainterPath barPath;
        barPath.addRoundedRect(rect.x() + 42, y + 2, qMax(barW, 2), barH - 4, 3, 3);
        p.drawPath(barPath);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 48 + qMax(barW, 2), y, 40, barH,
                   Qt::AlignVCenter, QString::number(count));
    }
}

void PaperAssetLedger2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Statistics");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Assets",   QString::number(entries_.size()),        QColor(0x3b, 0x82, 0xf6)},
        {"Total Value",    "$" + QString::number(totalValue(), 'f', 0),
                                                                    QColor(0xd9, 0x77, 0x06)},
        {"Active",         QString::number(activeCount()),          QColor(0x16, 0xa3, 0x4a)},
        {"Avg Value",      "$" + QString::number(
                               entries_.isEmpty() ? 0.0
                                   : totalValue() / entries_.size(), 'f', 0),
                                                                    QColor(0x7c, 0x3a, 0xed)},
    };

    int boxH = qMin(48, (rect.height() - 30) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 22 + i * (boxH + 6);

        // Background box
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath boxPath;
        boxPath.addRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.drawPath(boxPath);

        // Left accent bar
        p.setBrush(stats[i].color);
        QPainterPath accent;
        accent.addRoundedRect(rect.x(), y, 4, boxH, 2, 2);
        p.drawPath(accent);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 12, y + 2, rect.width() - 24, 24,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 26, rect.width() - 24, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperAssetLedger2::onTrade() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Equipment", "Software", "Data", "IP", "Facility"};
    QStringList statuses   = {"Active", "Frozen", "Depreciating", "Growing"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6), // Equipment - blue
        QColor(0x16, 0xa3, 0x4a), // Software  - green
        QColor(0xd9, 0x77, 0x06), // Data      - amber
        QColor(0xdc, 0x26, 0x26), // IP        - red
        QColor(0x7c, 0x3a, 0xed), // Facility  - purple
    };

    int cIdx = categoryCombo_->currentIndex();
    int catIndex = (cIdx == 0)
        ? QRandomGenerator::global()->bounded(categories.size())
        : cIdx - 1;

    AssetLedger2Entry entry;
    entry.id           = entries_.size() + 1;
    entry.asset        = text;
    entry.category     = categories[catIndex];
    entry.status       = statuses[QRandomGenerator::global()->bounded(statuses.size())];
    entry.value        = 100.0 + QRandomGenerator::global()->bounded(9901);
    entry.transactions = 1 + QRandomGenerator::global()->bounded(50);
    entry.active       = QRandomGenerator::global()->bounded(3) != 0; // ~67% active
    entry.color        = catColors[catIndex];

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assetTraded(entry.id, entry.value);
    inputField_->clear();
    update();
}

void PaperAssetLedger2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperAssetLedger2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Assets: 0 | Active: 0 | Total Value: $0");
        return;
    }
    infoLabel_->setText(QString("Assets: %1 | Active: %2 | Total Value: $%3")
        .arg(entries_.size())
        .arg(activeCount())
        .arg(totalValue(), 0, 'f', 0));
}

void PaperAssetLedger2::loadSettings() {
    settings_.beginGroup("AssetLedger2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AssetLedger2Entry e;
        e.id           = settings_.value("id").toInt();
        e.asset        = settings_.value("asset").toString();
        e.category     = settings_.value("category").toString();
        e.status       = settings_.value("status").toString();
        e.value        = settings_.value("value").toDouble();
        e.transactions = settings_.value("transactions").toInt();
        e.active       = settings_.value("active").toBool();
        e.color        = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    // Seed 8 entries if empty
    if (entries_.isEmpty()) {
        QStringList categories = {"Equipment", "Software", "Data", "IP", "Facility"};
        QStringList statuses   = {"Active", "Frozen", "Depreciating", "Growing"};
        QColor catColors[] = {
            QColor(0x3b, 0x82, 0xf6),
            QColor(0x16, 0xa3, 0x4a),
            QColor(0xd9, 0x77, 0x06),
            QColor(0xdc, 0x26, 0x26),
            QColor(0x7c, 0x3a, 0xed),
        };

        struct SeedData {
            const char* asset; int catIdx; const char* status;
            qreal val; int txns; bool active;
        };
        SeedData seeds[] = {
            {"SEM Microscope",   0, "Active",        2500.0,  23, true },
            {"MATLAB License",   1, "Depreciating",    800.0,  15, true },
            {"Genome Dataset",   2, "Growing",        3200.0,  42, true },
            {"CRISPR Patent",    3, "Active",         7500.0,   8, true },
            {"Clean Room B2",    4, "Frozen",         4000.0,   5, false},
            {"Oscilloscope",     0, "Active",          600.0,  31, true },
            {"Python Pipeline",  1, "Growing",        1500.0,  47, true },
            {"Neural Model v3",  3, "Depreciating",   2200.0,  12, false},
        };

        for (int i = 0; i < 8; ++i) {
            AssetLedger2Entry e;
            e.id           = i + 1;
            e.asset        = QString::fromUtf8(seeds[i].asset);
            e.category     = categories[seeds[i].catIdx];
            e.status       = QString::fromUtf8(seeds[i].status);
            e.value        = seeds[i].val;
            e.transactions = seeds[i].txns;
            e.active       = seeds[i].active;
            e.color        = catColors[seeds[i].catIdx];
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperAssetLedger2::saveSettings() {
    settings_.beginGroup("AssetLedger2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",           entries_[i].id);
        settings_.setValue("asset",        entries_[i].asset);
        settings_.setValue("category",     entries_[i].category);
        settings_.setValue("status",       entries_[i].status);
        settings_.setValue("value",        entries_[i].value);
        settings_.setValue("transactions", entries_[i].transactions);
        settings_.setValue("active",       entries_[i].active);
        settings_.setValue("color",        entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperAssetLedger2::addEntry(const AssetLedger2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assetTraded(entry.id, entry.value);
    update();
}

QList<AssetLedger2Entry> PaperAssetLedger2::entries() const {
    return entries_;
}

int PaperAssetLedger2::activeCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.active) ++c;
    return c;
}

qreal PaperAssetLedger2::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_)
        t += e.value;
    return t;
}

QMap<QString, int> PaperAssetLedger2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}
