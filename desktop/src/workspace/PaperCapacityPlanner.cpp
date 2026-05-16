#include "workspace/PaperCapacityPlanner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCapacityPlanner::PaperCapacityPlanner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CapacityPlanner")
{
    setupUI();
    loadSettings();
}

void PaperCapacityPlanner::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperCapacityPlanner::onUpdate);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Compute", "Storage", "Network", "Personnel", "Budget"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCapacityPlanner::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter resource name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Plan capacity");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperCapacityPlanner::addEntry(const CapacityEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit capacityUpdated(entry.id, entry.used);
    update();
}

QList<CapacityEntry> PaperCapacityPlanner::entries() const { return entries_; }

int PaperCapacityPlanner::overloadedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.overloaded) c++;
    return c;
}

qreal PaperCapacityPlanner::avgUtilization() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) {
        if (e.allocated > 0)
            sum += e.used / e.allocated;
        else
            sum += 0;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperCapacityPlanner::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCapacityPlanner::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"compute", "storage", "network", "personnel", "budget"};
    QStringList periods = {"Q1 2026", "Q2 2026", "Q3 2026", "Q4 2026", "H1 2026", "H2 2026"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        CapacityEntry e;
        e.id = entries_.size() + 1;
        e.resource = text.left(8) + " res" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.period = periods[QRandomGenerator::global()->bounded(periods.size())];
        e.allocated = 10.0 + QRandomGenerator::global()->bounded(90);
        e.used = 5.0 + QRandomGenerator::global()->bounded(95);
        e.used = qMin(e.used, e.allocated * 1.3);
        e.available = e.allocated - e.used;
        if (e.available < 0) e.available = 0;
        e.overloaded = e.used > e.allocated * 0.9;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCapacityPlanner::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Plan capacity");
    update();
}

void PaperCapacityPlanner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Plan capacity");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Capacity Planner");
    int w = width(), h = height();
    drawCapacityList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCapacityPlanner::drawCapacityList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.overloaded ? QColor(0xdc, 0x26, 0x26) : e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString tag = e.overloaded ? " [OVER]" : "";
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.resource.left(14) + tag);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        qreal utilPct = e.allocated > 0 ? (e.used / e.allocated * 100.0) : 0;
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.period + " | avail:" + QString::number(e.available, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(utilPct, 'f', 0) + "%");
        // Mini utilization bar
        int barY = y + 22;
        int barW = rect.width() / 2 - 10;
        int fillW = static_cast<int>(barW * qMin(utilPct, 100.0) / 100.0);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x() + rect.width() / 2, barY, barW, 6, 3, 3);
        p.setBrush(e.overloaded ? QColor(0xdc, 0x26, 0x26) : e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, barY, fillW, 6, 3, 3);
    }
}

void PaperCapacityPlanner::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"compute", "storage", "network", "personnel", "budget"};
    QString labels[] = {"Compute", "Storage", "Network", "Personnel", "Budget"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCapacityPlanner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Overloaded", QString::number(overloadedCount()), QColor(0xdc, 0x26, 0x26)},
        {"Avg Util", QString::number(avgUtilization() * 100, 'f', 0) + "%", QColor(0xd9, 0x77, 0x06)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c, 0x3a, 0xed)}
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

void PaperCapacityPlanner::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Plan capacity"); return; }
    infoLabel_->setText(QString("%1 entries | %2 overloaded | %3% util")
        .arg(entries_.size())
        .arg(overloadedCount())
        .arg(avgUtilization() * 100, 0, 'f', 0));
}

void PaperCapacityPlanner::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CapacityEntry e;
        e.id = settings_.value("id").toInt();
        e.resource = settings_.value("resource").toString();
        e.category = settings_.value("category").toString();
        e.period = settings_.value("period").toString();
        e.allocated = settings_.value("allocated").toDouble();
        e.used = settings_.value("used").toDouble();
        e.available = settings_.value("available").toDouble();
        e.overloaded = settings_.value("overloaded").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCapacityPlanner::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("resource", entries_[i].resource);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("allocated", entries_[i].allocated);
        settings_.setValue("used", entries_[i].used);
        settings_.setValue("available", entries_[i].available);
        settings_.setValue("overloaded", entries_[i].overloaded);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
