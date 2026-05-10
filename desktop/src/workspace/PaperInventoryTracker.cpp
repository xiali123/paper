#include "workspace/PaperInventoryTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperInventoryTracker::PaperInventoryTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "InventoryTracker")
{
    setupUI();
    loadSettings();
}

void PaperInventoryTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperInventoryTracker::onTrack);
    toolbar->addWidget(trackBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Equipment", "Software", "Books", "Materials"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperInventoryTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter item name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Track inventory");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperInventoryTracker::addEntry(const InventoryEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit inventoryUpdated(entry.id, entry.quantity);
    update();
}

QList<InventoryEntry> PaperInventoryTracker::entries() const { return entries_; }

qreal PaperInventoryTracker::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value * e.quantity;
    return t;
}

int PaperInventoryTracker::lowStockCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.lowStock) c++;
    return c;
}

QMap<QString, int> PaperInventoryTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperInventoryTracker::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"equipment", "software", "books", "materials"};
    QStringList locations = {"lab-A", "lab-B", "office", "storage"};
    QStringList statuses = {"in-stock", "in-use", "reserved", "maintenance"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        InventoryEntry e;
        e.id = entries_.size() + 1;
        e.itemName = text.left(8) + " item" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.quantity = QRandomGenerator::global()->bounded(50);
        e.threshold = 5 + QRandomGenerator::global()->bounded(10);
        e.location = locations[QRandomGenerator::global()->bounded(locations.size())];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.value = 10 + QRandomGenerator::global()->bounded(5000);
        e.lowStock = e.quantity <= e.threshold;
        e.color = e.lowStock ? QColor(239,68,68) : (e.status == "in-use" ? QColor(59,130,246) : QColor(16,185,129));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperInventoryTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track inventory");
    update();
}

void PaperInventoryTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track inventory");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Inventory Tracker");
    int w = width(), h = height();
    drawInventoryList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperInventoryTracker::drawInventoryList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.itemName.left(14) + (e.lowStock ? " [LOW]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.location + " | " + e.status);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.quantity) + "/" + QString::number(e.threshold));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.value, 'f', 0) + " ea");
    }
}

void PaperInventoryTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"equipment", "software", "books", "materials"};
    QString labels[] = {"Equip", "Soft", "Books", "Matl"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
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

void PaperInventoryTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"Low Stock", QString::number(lowStockCount()), QColor(239,68,68)},
        {"Total Val", "$" + QString::number(totalValue(), 'f', 0), QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
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

void PaperInventoryTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track inventory"); return; }
    infoLabel_->setText(QString("%1 items | %2 low | $%3 val")
        .arg(entries_.size()).arg(lowStockCount()).arg(totalValue(), 0, 'f', 0));
}

void PaperInventoryTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        InventoryEntry e;
        e.id = settings_.value("id").toInt();
        e.itemName = settings_.value("itemName").toString();
        e.category = settings_.value("category").toString();
        e.quantity = settings_.value("quantity").toInt();
        e.threshold = settings_.value("threshold").toInt();
        e.location = settings_.value("location").toString();
        e.status = settings_.value("status").toString();
        e.value = settings_.value("value").toDouble();
        e.lowStock = settings_.value("lowStock").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperInventoryTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("itemName", entries_[i].itemName);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("quantity", entries_[i].quantity);
        settings_.setValue("threshold", entries_[i].threshold);
        settings_.setValue("location", entries_[i].location);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("lowStock", entries_[i].lowStock);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
