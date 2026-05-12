#include "workspace/PaperAssetInventory.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperAssetInventory::PaperAssetInventory(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AssetInventory")
{
    setupUI();
    loadSettings();
}

void PaperAssetInventory::setupUI() {
    auto* layout = new QHBoxLayout(this);
    auto* left = new QWidget();
    auto* leftLayout = new QHBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Equipment", "Software", "Books", "Materials", "Furniture"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Asset name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftLayout->addWidget(inputField_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperAssetInventory::onAdd);
    leftLayout->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAssetInventory::onClear);
    leftLayout->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Assets: 0 | Tracked: 0 | Total Value: $0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftLayout->addWidget(infoLabel_);

    layout->addWidget(left);
    layout->addStretch();
    setMinimumSize(640, 520);
}

void PaperAssetInventory::addEntry(const AssetEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assetAdded(entry.id, entry.value);
    update();
}

QList<AssetEntry> PaperAssetInventory::entries() const { return entries_; }

int PaperAssetInventory::trackedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.tracked) ++c;
    return c;
}

qreal PaperAssetInventory::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value * e.quantity;
    return t;
}

QMap<QString, int> PaperAssetInventory::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperAssetInventory::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"equipment", "software", "books", "materials", "furniture"};
    QStringList locations = {"lab-A", "lab-B", "office", "storage", "server-room"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int cIdx = categoryCombo_->currentIndex();
    AssetEntry e;
    e.id = entries_.size() + 1;
    e.name = text;
    e.category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    e.value = 100 + QRandomGenerator::global()->bounded(9901);
    e.quantity = 1 + QRandomGenerator::global()->bounded(50);
    e.tracked = QRandomGenerator::global()->bounded(2) == 0;
    e.location = locations[QRandomGenerator::global()->bounded(locations.size())];
    e.color = palette[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
    inputField_->clear();
}

void PaperAssetInventory::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperAssetInventory::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No assets");
        return;
    }

    int w = width(), h = height();
    drawAssetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAssetInventory::drawAssetList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 8, "Asset Inventory");

    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    qreal maxVal = 1;
    for (int i = 0; i < show; ++i)
        maxVal = qMax(maxVal, entries_[i].value);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // background bar proportional to value
        int barW = static_cast<int>((e.value / maxVal) * (rect.width() - 4));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, barW, itemH, 4, 4);

        // color indicator stripe
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // name + tracked badge
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString label = e.name.left(14) + (e.tracked ? " [TRACKED]" : "");
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2, 16, Qt::AlignVCenter, label);

        // details line
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2, 14, Qt::AlignVCenter,
                   e.category + " | " + e.location + " | x" + QString::number(e.quantity));

        // value
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.value, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "Total: $" + QString::number(e.value * e.quantity, 'f', 0));
    }
}

void PaperAssetInventory::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"equipment", "software", "books", "materials", "furniture"};
    QString labels[] = {"Equip", "Soft", "Books", "Matl", "Furn"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 5);

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperAssetInventory::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Assets", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Tracked",     QString::number(trackedCount()),  QColor(0x16, 0xa3, 0x4a)},
        {"Total Value", "$" + QString::number(totalValue(), 'f', 0), QColor(0xd9, 0x77, 0x06)}
    };
    int boxH = qMin(48, (rect.height() - 10) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperAssetInventory::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Assets: 0 | Tracked: 0 | Total Value: $0");
        return;
    }
    infoLabel_->setText(QString("Assets: %1 | Tracked: %2 | Total Value: $%3")
        .arg(entries_.size())
        .arg(trackedCount())
        .arg(totalValue(), 0, 'f', 0));
}

void PaperAssetInventory::loadSettings() {
    settings_.beginGroup("AssetInventory");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AssetEntry e;
        e.id       = settings_.value("id").toInt();
        e.name     = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.location = settings_.value("location").toString();
        e.value    = settings_.value("value").toDouble();
        e.quantity = settings_.value("quantity").toInt();
        e.tracked  = settings_.value("tracked").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperAssetInventory::saveSettings() {
    settings_.beginGroup("AssetInventory");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("name",     entries_[i].name);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("location", entries_[i].location);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("quantity", entries_[i].quantity);
        settings_.setValue("tracked",  entries_[i].tracked);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
