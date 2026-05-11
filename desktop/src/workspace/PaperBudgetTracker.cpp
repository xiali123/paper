#include "workspace/PaperBudgetTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBudgetTracker::PaperBudgetTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BudgetTracker")
{
    setupUI();
    loadSettings();
}

void PaperBudgetTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperBudgetTracker::onAdd);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Equipment", "Travel", "Software", "Personnel"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBudgetTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter budget item...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Track budget");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperBudgetTracker::addEntry(const BudgetEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit budgetUpdated(entry.id, entry.remaining);
    update();
}

QList<BudgetEntry> PaperBudgetTracker::entries() const { return entries_; }

int PaperBudgetTracker::overBudgetCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.overBudget) c++;
    return c;
}

qreal PaperBudgetTracker::totalSpent() const {
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.spent;
    return sum;
}

QMap<QString, int> PaperBudgetTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBudgetTracker::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"equipment", "travel", "software", "personnel"};
    QStringList types = {"capex", "opex"};
    QStringList items = {"GPU cluster", "conference trip", "MATLAB license", "RA salary",
                         "laptop", "cloud storage", "journal sub", "postdoc stipend"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        BudgetEntry e;
        e.id = entries_.size() + 1;
        e.item = items[QRandomGenerator::global()->bounded(items.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.type = types[QRandomGenerator::global()->bounded(types.size())];
        e.allocated = 500 + QRandomGenerator::global()->bounded(10000);
        e.spent = QRandomGenerator::global()->bounded(static_cast<int>(e.allocated) + 1000);
        e.remaining = e.allocated - e.spent;
        e.overBudget = e.remaining < 0;
        if (e.overBudget)
            e.color = QColor(239,68,68);
        else if (e.remaining > 0)
            e.color = QColor(16,185,129);
        else
            e.color = QColor(245,158,11);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperBudgetTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track budget");
    update();
}

void PaperBudgetTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track budget");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Budget Tracker");
    int w = width(), h = height();
    drawBudgetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBudgetTracker::drawBudgetList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(40, (rect.height() - 10) / qMax(show, 1));
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
        p.drawText(rect.x() + 10, y + 3, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.item + (e.overBudget ? " [OVER]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 17, rect.width() / 2 - 10, 12, Qt::AlignVCenter,
                   e.type + " | " + e.category);

        // Progress bar for spent vs allocated
        int barY = y + 30;
        int barW = rect.width() / 2 - 10;
        qreal ratio = e.allocated > 0 ? qMin(e.spent / e.allocated, 1.5) : 0;
        int fillW = qMin(static_cast<int>(ratio * barW), barW);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x() + 10, barY, barW, 6, 3, 3);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 10, barY, fillW, 6, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 3, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.allocated, 'f', 0) + " alloc");
        p.drawText(rect.x() + rect.width() / 2, y + 17, rect.width() / 2 - 10, 12,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.spent, 'f', 0) + " spent");
        QString remText = e.remaining >= 0
            ? "$" + QString::number(e.remaining, 'f', 0) + " left"
            : "$" + QString::number(qAbs(e.remaining), 'f', 0) + " over";
        p.drawText(rect.x() + rect.width() / 2, y + 30, rect.width() / 2 - 10, 12,
                   Qt::AlignVCenter | Qt::AlignRight, remText);
    }
}

void PaperBudgetTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"equipment", "travel", "software", "personnel"};
    QString labels[] = {"Equip", "Travel", "Softw", "Person"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperBudgetTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"Over Budget", QString::number(overBudgetCount()), QColor(239,68,68)},
        {"Total Spent", "$" + QString::number(totalSpent(), 'f', 0), QColor(245,158,11)},
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

void PaperBudgetTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track budget"); return; }
    infoLabel_->setText(QString("%1 items | %2 over | $%3 spent")
        .arg(entries_.size()).arg(overBudgetCount()).arg(totalSpent(), 0, 'f', 0));
}

void PaperBudgetTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BudgetEntry e;
        e.id = settings_.value("id").toInt();
        e.item = settings_.value("item").toString();
        e.category = settings_.value("category").toString();
        e.type = settings_.value("type").toString();
        e.allocated = settings_.value("allocated").toDouble();
        e.spent = settings_.value("spent").toDouble();
        e.remaining = settings_.value("remaining").toDouble();
        e.overBudget = settings_.value("overBudget").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBudgetTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("item", entries_[i].item);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("allocated", entries_[i].allocated);
        settings_.setValue("spent", entries_[i].spent);
        settings_.setValue("remaining", entries_[i].remaining);
        settings_.setValue("overBudget", entries_[i].overBudget);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
