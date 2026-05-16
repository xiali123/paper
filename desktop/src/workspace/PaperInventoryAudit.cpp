#include "workspace/PaperInventoryAudit.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

namespace {
const QColor kBlue   = QColor(0x3b, 0x82, 0xf6);
const QColor kGreen  = QColor(0x16, 0xa3, 0x4a);
const QColor kAmber  = QColor(0xd9, 0x77, 0x06);
const QColor kRed    = QColor(0xdc, 0x26, 0x26);
const QColor kPurple = QColor(0x7c, 0x3a, 0xed);

QStringList kItems     = {"Lab Equipment", "Reagents", "Safety Gear", "Software Licenses", "Office Supplies"};
QStringList kStatuses  = {"OK", "Warning", "Critical", "Pending"};
QStringList kCategories = {"Lab", "IT", "Office", "Safety", "General"};

QColor statusColor(const QString& status) {
    if (status == "OK")      return kGreen;
    if (status == "Warning") return kAmber;
    if (status == "Critical")return kRed;
    return kBlue; // Pending
}
} // anonymous namespace

PaperInventoryAudit::PaperInventoryAudit(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "InventoryAudit")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        for (int i = 0; i < 8; ++i) {
            InventoryAuditEntry e;
            e.id = i + 1;
            e.item = kItems[QRandomGenerator::global()->bounded(kItems.size())];
            e.category = kCategories[QRandomGenerator::global()->bounded(kCategories.size())];
            e.status = kStatuses[QRandomGenerator::global()->bounded(kStatuses.size())];
            e.quantity = 5.0 + QRandomGenerator::global()->bounded(200);
            e.discrepancies = QRandomGenerator::global()->bounded(12);
            e.flagged = e.discrepancies > 5;
            e.color = statusColor(e.status);
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperInventoryAudit::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Lab", "IT", "Office", "Safety", "General"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search items...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    auditBtn_ = new QPushButton("Audit");
    auditBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(auditBtn_, &QPushButton::clicked, this, &PaperInventoryAudit::onAudit);
    toolbar->addWidget(auditBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperInventoryAudit::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel;
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperInventoryAudit::addEntry(const InventoryAuditEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    if (entry.flagged)
        emit auditFlagged(entry.id, entry.discrepancies);
    update();
}

QList<InventoryAuditEntry> PaperInventoryAudit::entries() const { return entries_; }

int PaperInventoryAudit::flaggedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.flagged) ++c;
    return c;
}

qreal PaperInventoryAudit::totalQuantity() const {
    qreal t = 0;
    for (const auto& e : entries_)
        t += e.quantity;
    return t;
}

QMap<QString, int> PaperInventoryAudit::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperInventoryAudit::onAudit() {
    InventoryAuditEntry e;
    e.id = entries_.size() + 1;
    e.item = kItems[QRandomGenerator::global()->bounded(kItems.size())];
    int catIdx = categoryCombo_->currentIndex();
    e.category = (catIdx == 0)
        ? kCategories[QRandomGenerator::global()->bounded(kCategories.size())]
        : kCategories[catIdx - 1];
    e.status = kStatuses[QRandomGenerator::global()->bounded(kStatuses.size())];
    e.quantity = 1.0 + QRandomGenerator::global()->bounded(250);
    e.discrepancies = QRandomGenerator::global()->bounded(15);
    e.flagged = e.discrepancies > 5;
    e.color = statusColor(e.status);
    addEntry(e);
}

void PaperInventoryAudit::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperInventoryAudit::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No audit entries");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Inventory Audit");

    int w = width(), h = height();

    // Left 60% height: audit view
    int auditH = static_cast<int>(h * 0.60);
    drawAuditView(p, QRect(20, 50, w - 40, auditH - 50));

    // Right 40% width: category chart (beside or below audit depending on layout)
    // Use right half of the middle band
    int chartX = w / 2 + 10;
    int chartY = auditH + 10;
    int chartH = static_cast<int>(h * 0.15);
    drawCategoryChart(p, QRect(chartX, chartY, w / 2 - 30, chartH));

    // Bottom 25%: stats
    int statsY = h - static_cast<int>(h * 0.25);
    drawStats(p, QRect(20, statsY, w - 40, h - statsY - 10));
}

void PaperInventoryAudit::drawAuditView(QPainter& p, const QRect& rect) {
    // Filter by category and search text
    QString filter = inputField_->text().trimmed().toLower();
    int catIdx = categoryCombo_->currentIndex();
    QString catFilter = (catIdx > 0) ? kCategories[catIdx - 1] : QString();

    struct VisibleEntry { const InventoryAuditEntry* e; };
    QList<VisibleEntry> visible;
    for (const auto& e : entries_) {
        if (!catFilter.isEmpty() && e.category != catFilter) continue;
        if (!filter.isEmpty() && !e.item.toLower().contains(filter)) continue;
        visible.append({&e});
    }

    int show = qMin(10, visible.size());
    if (show == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching entries");
        return;
    }

    // Header row
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8, QFont::Bold));
    int col0 = rect.x();           // item name
    int col1 = rect.x() + rect.width() * 0.30; // status badge
    int col2 = rect.x() + rect.width() * 0.50; // quantity bar
    int col3 = rect.x() + rect.width() * 0.75; // discrepancies
    int col4 = rect.x() + rect.width() * 0.90; // flag
    p.drawText(col0, rect.y(), 100, 18, Qt::AlignVCenter, "Item");
    p.drawText(col1, rect.y(), 80, 18, Qt::AlignVCenter, "Status");
    p.drawText(col2, rect.y(), 80, 18, Qt::AlignVCenter, "Quantity");
    p.drawText(col3, rect.y(), 60, 18, Qt::AlignVCenter, "Disc.");
    p.drawText(col4, rect.y(), 40, 18, Qt::AlignVCenter, "Flag");

    int itemH = qMin(30, (rect.height() - 24) / qMax(show, 1));
    qreal maxQty = 1.0;
    for (int i = 0; i < show; ++i)
        maxQty = qMax(maxQty, visible[i].e->quantity);

    for (int i = 0; i < show; ++i) {
        const auto& e = *visible[i].e;
        int y = rect.y() + 22 + i * (itemH + 3);

        // Row background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left color indicator bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Item name + category
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(col0 + 8, y, col1 - col0 - 12, itemH, Qt::AlignVCenter,
                   e.item.left(20));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(col0 + 8, y + 14, col1 - col0 - 12, itemH - 14, Qt::AlignVCenter,
                   e.category);

        // Status badge
        QColor sc = statusColor(e.status);
        p.setPen(Qt::NoPen);
        p.setBrush(sc);
        p.drawRoundedRect(col1, y + 4, 60, itemH - 8, 10, 10);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(col1, y + 4, 60, itemH - 8, Qt::AlignCenter, e.status);

        // Quantity bar
        int barMaxW = col3 - col2 - 10;
        int barW = static_cast<int>((e.quantity / maxQty) * barMaxW);
        p.setPen(Qt::NoPen);
        p.setBrush(kBlue.lighter(170));
        p.drawRoundedRect(col2, y + 6, barMaxW, itemH - 12, 3, 3);
        p.setBrush(kBlue);
        p.drawRoundedRect(col2, y + 6, qMax(barW, 2), itemH - 12, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(col2 + barMaxW + 2, y, 40, itemH, Qt::AlignVCenter,
                   QString::number(static_cast<int>(e.quantity)));

        // Discrepancy count
        p.setPen(e.discrepancies > 5 ? kRed : QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(col3, y, 50, itemH, Qt::AlignVCenter,
                   QString::number(e.discrepancies));

        // Flag indicator
        if (e.flagged) {
            p.setPen(Qt::NoPen);
            p.setBrush(kRed);
            p.drawEllipse(col4 + 5, y + itemH / 2 - 5, 10, 10);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(col4 + 5, y + itemH / 2 - 5, 10, 10, Qt::AlignCenter, "!");
        }
    }
}

void PaperInventoryAudit::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Distribution");

    // Count statuses per category
    QMap<QString, QMap<QString, int>> data;
    for (const auto& cat : kCategories)
        for (const auto& st : kStatuses)
            data[cat][st] = 0;
    for (const auto& e : entries_)
        data[e.category][e.status]++;

    int maxVal = 1;
    for (const auto& cat : kCategories) {
        int sum = 0;
        for (const auto& st : kStatuses)
            sum += data[cat][st];
        maxVal = qMax(maxVal, sum);
    }

    int barH = qMin(20, (rect.height() - 30) / kCategories.size());
    QColor statusColors[] = {kGreen, kAmber, kRed, kBlue};
    QString statusLabels[] = {"OK", "Warning", "Critical", "Pending"};

    for (int i = 0; i < kCategories.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int labelW = 40;

        // Category label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter,
                   kCategories[i]);

        // Stacked bar
        int barX = rect.x() + labelW + 5;
        int barMaxW = rect.width() - labelW - 30;
        int x = barX;
        for (int s = 0; s < 4; ++s) {
            int count = data[kCategories[i]][statusLabels[s]];
            int segW = static_cast<int>((static_cast<qreal>(count) / maxVal) * barMaxW);
            if (segW > 0) {
                p.setPen(Qt::NoPen);
                p.setBrush(statusColors[s]);
                p.drawRoundedRect(x, y, segW, barH - 2, 2, 2);
                x += segW;
            }
        }
    }

    // Legend
    int legY = rect.y() + rect.height() - 12;
    int legX = rect.x() + 50;
    p.setFont(QFont("Arial", 7));
    for (int s = 0; s < 4; ++s) {
        p.setPen(Qt::NoPen);
        p.setBrush(statusColors[s]);
        p.drawRoundedRect(legX, legY, 8, 8, 2, 2);
        p.setPen(QColor(100, 116, 139));
        p.drawText(legX + 10, legY + 8, statusLabels[s]);
        legX += 70;
    }
}

void PaperInventoryAudit::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Items",     QString::number(entries_.size()),      kBlue},
        {"Flagged Count",   QString::number(flaggedCount()),       kRed},
        {"Total Quantity",  QString::number(totalQuantity(), 'f', 0), kAmber},
        {"Avg Discrepancies", entries_.isEmpty()
            ? "0.0"
            : QString::number(static_cast<qreal>(
                  std::accumulate(entries_.begin(), entries_.end(), 0,
                      [](int s, const InventoryAuditEntry& e) { return s + e.discrepancies; }))
                  / entries_.size(), 'f', 1),
            kPurple}
    };

    int boxW = (rect.width() - 30) / 4;
    int boxH = rect.height() - 4;
    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y();

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(x + 10, y + 5, boxW - 20, boxH / 2, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + boxH / 2, boxW - 20, boxH / 2 - 4, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperInventoryAudit::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No entries");
        return;
    }
    infoLabel_->setText(
        QString("%1 items | %2 flagged | Qty: %3 | Avg disc: %4")
            .arg(entries_.size())
            .arg(flaggedCount())
            .arg(totalQuantity(), 0, 'f', 0)
            .arg(entries_.isEmpty() ? 0.0
                : static_cast<qreal>(
                    std::accumulate(entries_.begin(), entries_.end(), 0,
                        [](int s, const InventoryAuditEntry& e) { return s + e.discrepancies; }))
                    / entries_.size(),
                1, 'f', 1));
}

void PaperInventoryAudit::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        InventoryAuditEntry e;
        e.id           = settings_.value("id").toInt();
        e.item         = settings_.value("item").toString();
        e.category     = settings_.value("category").toString();
        e.status       = settings_.value("status").toString();
        e.quantity     = settings_.value("quantity").toDouble();
        e.discrepancies= settings_.value("discrepancies").toInt();
        e.flagged      = settings_.value("flagged").toBool();
        e.color        = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperInventoryAudit::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",           entries_[i].id);
        settings_.setValue("item",         entries_[i].item);
        settings_.setValue("category",     entries_[i].category);
        settings_.setValue("status",       entries_[i].status);
        settings_.setValue("quantity",     entries_[i].quantity);
        settings_.setValue("discrepancies",entries_[i].discrepancies);
        settings_.setValue("flagged",      entries_[i].flagged);
        settings_.setValue("color",        entries_[i].color.name());
    }
    settings_.endArray();
}
