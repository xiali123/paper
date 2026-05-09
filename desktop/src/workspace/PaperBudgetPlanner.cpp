#include "workspace/PaperBudgetPlanner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperBudgetPlanner::PaperBudgetPlanner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BudgetPlanner")
{
    setupUI();
    loadSettings();
}

void PaperBudgetPlanner::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Item");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperBudgetPlanner::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Conference", "Software", "Equipment", "Travel"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBudgetPlanner::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Plan research budget");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperBudgetPlanner::addEntry(const BudgetEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit budgetUpdated(totalBudget());
    update();
}

QList<BudgetEntry> PaperBudgetPlanner::entries() const { return entries_; }

QMap<QString, int> PaperBudgetPlanner::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

qreal PaperBudgetPlanner::totalBudget() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.amount;
    return t;
}

qreal PaperBudgetPlanner::totalSpent() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.spent;
    return t;
}

void PaperBudgetPlanner::onAdd() {
    bool ok;
    QString item = QInputDialog::getText(this, "Add Item", "Item:", QLineEdit::Normal, "", &ok);
    if (!ok || item.isEmpty()) return;

    QStringList cats = {"conference", "software", "equipment", "travel"};
    QString cat = QInputDialog::getItem(this, "Add Item", "Category:", cats, 0, false, &ok);
    if (!ok) return;

    BudgetEntry e;
    e.id = entries_.size() + 1;
    e.item = item;
    e.category = cat;
    e.amount = 100 + QRandomGenerator::global()->bounded(4900);
    e.spent = QRandomGenerator::global()->bounded(static_cast<int>(e.amount));
    e.status = e.spent >= e.amount ? "over" : (e.spent > 0 ? "partial" : "planned");
    e.deadline = QDate::currentDate().addDays(7 + QRandomGenerator::global()->bounded(90)).toString("MM/dd");
    e.notes = "Budget for " + item.left(12);

    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int cIdx = cats.indexOf(cat);
    e.color = catColors[qBound(0, cIdx, 3)];
    addEntry(e);
}

void PaperBudgetPlanner::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Plan research budget");
    update();
}

void PaperBudgetPlanner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Plan research budget");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Budget Planner");

    int w = width(), h = height();
    drawBudgetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBudgetPlanner::drawBudgetList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = 0; i < entries_.size() && show < maxShow; ++i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.category != "conference") continue;
        if (filterIdx == 2 && e.category != "software") continue;
        if (filterIdx == 3 && e.category != "equipment") continue;
        if (filterIdx == 4 && e.category != "travel") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.item.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.status + " | " + e.deadline);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(static_cast<int>(e.amount)));

        int barW = static_cast<int>((e.spent / qMax(e.amount, 1.0)) * (rect.width() / 2 - 14));
        p.setPen(Qt::NoPen);
        p.setBrush(e.spent > e.amount * 0.8 ? QColor(239,68,68) : e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 22, barW, 8, 3, 3);
        show++;
    }
}

void PaperBudgetPlanner::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = {"conference", "software", "equipment", "travel"};
    QString labels[] = {"Conference", "Software", "Equipment", "Travel"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 3, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperBudgetPlanner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"Budget", "$" + QString::number(static_cast<int>(totalBudget())), QColor(16,185,129)},
        {"Spent", "$" + QString::number(static_cast<int>(totalSpent())), QColor(245,158,11)},
        {"Remaining", "$" + QString::number(static_cast<int>(totalBudget() - totalSpent())), QColor(139,92,246)}
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

void PaperBudgetPlanner::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Plan research budget"); return; }
    infoLabel_->setText(QString("%1 items | $%2 budget | $%3 spent")
        .arg(entries_.size()).arg(static_cast<int>(totalBudget())).arg(static_cast<int>(totalSpent())));
}

void PaperBudgetPlanner::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BudgetEntry e;
        e.id = settings_.value("id").toInt();
        e.item = settings_.value("item").toString();
        e.category = settings_.value("category").toString();
        e.amount = settings_.value("amount").toDouble();
        e.spent = settings_.value("spent").toDouble();
        e.status = settings_.value("status").toString();
        e.deadline = settings_.value("deadline").toString();
        e.notes = settings_.value("notes").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBudgetPlanner::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("item", entries_[i].item);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("amount", entries_[i].amount);
        settings_.setValue("spent", entries_[i].spent);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("deadline", entries_[i].deadline);
        settings_.setValue("notes", entries_[i].notes);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
