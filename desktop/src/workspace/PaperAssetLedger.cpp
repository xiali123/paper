#include "workspace/PaperAssetLedger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperAssetLedger::PaperAssetLedger(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AssetLedger")
{
    setupUI();
    loadSettings();
}

void PaperAssetLedger::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Hardware", "Software", "License", "Document"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 100px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter asset...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    recordBtn_ = new QPushButton("Record");
    recordBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(recordBtn_, &QPushButton::clicked, this, &PaperAssetLedger::onRecord);
    toolbar->addWidget(recordBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 4px 12px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAssetLedger::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Assets: 0 | Verified: 0 | Total Value: $0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 2px 4px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(720, 560);
}

void PaperAssetLedger::paintEvent(QPaintEvent*) {
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

void PaperAssetLedger::drawLedgerView(QPainter& p, const QRect& rect) {
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
    int rowH = 32;
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + 4, headerY, 120, 16, Qt::AlignVCenter, "Asset");
    p.drawText(rect.x() + 130, headerY, 80, 16, Qt::AlignVCenter, "Custodian");
    p.drawText(rect.x() + 220, headerY, 40, 16, Qt::AlignVCenter, "Qty");
    p.drawText(rect.x() + 270, headerY, 60, 16, Qt::AlignVCenter, "Value");
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
        p.drawText(rect.x() + 10, y, 115, rowH, Qt::AlignVCenter,
                   e.asset.left(16));

        // Custodian
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 130, y, 80, rowH, Qt::AlignVCenter,
                   e.custodian.left(10));

        // Quantity
        p.drawText(rect.x() + 220, y, 40, rowH, Qt::AlignVCenter,
                   "x" + QString::number(e.quantity));

        // Value
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 270, y, 60, rowH, Qt::AlignVCenter,
                   "$" + QString::number(e.value, 'f', 0));

        // Verified checkmark or dash
        if (e.verified) {
            p.setPen(QColor(22, 163, 74));
            p.setFont(QFont("Arial", 11, QFont::Bold));
            p.drawText(rect.x() + rect.width() - 60, y, 50, rowH,
                       Qt::AlignVCenter | Qt::AlignRight, QString::fromUtf8("\xe2\x9c\x93"));
        } else {
            p.setPen(QColor(203, 213, 225));
            p.setFont(QFont("Arial", 11));
            p.drawText(rect.x() + rect.width() - 60, y, 50, rowH,
                       Qt::AlignVCenter | Qt::AlignRight, "--");
        }
    }
}

void PaperAssetLedger::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Breakdown");

    auto counts = categoryCounts();
    struct CatInfo { QString key; QString label; QColor color; };
    CatInfo cats[] = {
        {"Hardware", "HW",     QColor(0x3b, 0x82, 0xf6)},
        {"Software", "SW",     QColor(0x16, 0xa3, 0x4a)},
        {"License",  "Lic",    QColor(0x7c, 0x3a, 0xed)},
        {"Document", "Doc",    QColor(0xd9, 0x77, 0x06)},
    };

    int maxCount = 1;
    for (const auto& c : cats) {
        int v = counts.contains(c.key) ? counts[c.key] : 0;
        maxCount = qMax(maxCount, v);
    }

    int barH = qMin(28, (rect.height() - 30) / 4);
    int chartW = rect.width() - 80;

    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i].key) ? counts[cats[i].key] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxCount) * chartW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 30, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i].label);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(cats[i].color);
        QPainterPath barPath;
        barPath.addRoundedRect(rect.x() + 36, y + 2, qMax(barW, 2), barH - 4, 3, 3);
        p.drawPath(barPath);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 42 + qMax(barW, 2), y, 40, barH,
                   Qt::AlignVCenter, QString::number(count));
    }
}

void PaperAssetLedger::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Statistics");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Assets",  QString::number(entries_.size()),     QColor(0x3b, 0x82, 0xf6)},
        {"Total Value",   "$" + QString::number(totalValue(), 'f', 0),
                                                            QColor(0xd9, 0x77, 0x06)},
        {"Verified",      QString::number(verifiedCount()),    QColor(0x16, 0xa3, 0x4a)},
    };

    int boxH = qMin(52, (rect.height() - 30) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 22 + i * (boxH + 6);

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
        p.setFont(QFont("Arial", 15, QFont::Bold));
        p.drawText(rect.x() + 12, y + 4, rect.width() - 24, 24,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 28, rect.width() - 24, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperAssetLedger::onRecord() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Hardware", "Software", "License", "Document"};
    QStringList custodians = {"Lab-A", "Lab-B", "Office", "Server", "Storage"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6), // Hardware
        QColor(0x16, 0xa3, 0x4a), // Software
        QColor(0x7c, 0x3a, 0xed), // License
        QColor(0xd9, 0x77, 0x06), // Document
    };

    int cIdx = categoryCombo_->currentIndex();
    int catIndex = (cIdx == 0)
        ? QRandomGenerator::global()->bounded(categories.size())
        : cIdx - 1;

    AssetLedgerEntry entry;
    entry.id       = entries_.size() + 1;
    entry.asset    = text;
    entry.category = categories[catIndex];
    entry.custodian = custodians[QRandomGenerator::global()->bounded(custodians.size())];
    entry.value    = 50.0 + QRandomGenerator::global()->bounded(9951);
    entry.quantity = 1 + QRandomGenerator::global()->bounded(25);
    entry.verified = QRandomGenerator::global()->bounded(3) != 0; // ~67% verified
    entry.color    = catColors[catIndex];

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assetRecorded(entry.id, entry.value);
    inputField_->clear();
    update();
}

void PaperAssetLedger::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperAssetLedger::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Assets: 0 | Verified: 0 | Total Value: $0");
        return;
    }
    infoLabel_->setText(QString("Assets: %1 | Verified: %2 | Total Value: $%3")
        .arg(entries_.size())
        .arg(verifiedCount())
        .arg(totalValue(), 0, 'f', 0));
}

void PaperAssetLedger::loadSettings() {
    settings_.beginGroup("AssetLedger");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AssetLedgerEntry e;
        e.id       = settings_.value("id").toInt();
        e.asset    = settings_.value("asset").toString();
        e.category = settings_.value("category").toString();
        e.custodian = settings_.value("custodian").toString();
        e.value    = settings_.value("value").toDouble();
        e.quantity = settings_.value("quantity").toInt();
        e.verified = settings_.value("verified").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperAssetLedger::saveSettings() {
    settings_.beginGroup("AssetLedger");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("asset",    entries_[i].asset);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("custodian", entries_[i].custodian);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("quantity", entries_[i].quantity);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperAssetLedger::addEntry(const AssetLedgerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assetRecorded(entry.id, entry.value);
    update();
}

QList<AssetLedgerEntry> PaperAssetLedger::entries() const {
    return entries_;
}

int PaperAssetLedger::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.verified) ++c;
    return c;
}

qreal PaperAssetLedger::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_)
        t += e.value * e.quantity;
    return t;
}

QMap<QString, int> PaperAssetLedger::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}
