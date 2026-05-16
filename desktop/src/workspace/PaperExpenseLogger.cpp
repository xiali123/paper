#include "workspace/PaperExpenseLogger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperExpenseLogger::PaperExpenseLogger(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ExpenseLogger")
{
    setupUI();
    loadSettings();
}

void PaperExpenseLogger::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Expense");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperExpenseLogger::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Publishing", "Equipment", "Travel", "Software", "Services"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperExpenseLogger::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter expense description...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Log research expenses");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperExpenseLogger::addEntry(const ExpenseEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit expenseLogged(entry.id, entry.amount);
    update();
}

QList<ExpenseEntry> PaperExpenseLogger::entries() const { return entries_; }

qreal PaperExpenseLogger::totalAmount() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.amount;
    return t;
}

int PaperExpenseLogger::reimbursableCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.reimbursable) c++;
    return c;
}

QMap<QString, int> PaperExpenseLogger::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperExpenseLogger::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"publishing", "equipment", "travel", "software", "services"};
    QStringList dates = {"2026-05-01", "2026-05-05", "2026-05-10", "2026-05-15"};
    QStringList projects = {"Paper A", "Paper B", "Paper C"};
    QStringList receipts = {"yes", "no", "pending"};

    int cIdx = categoryCombo_->currentIndex();
    ExpenseEntry e;
    e.id = entries_.size() + 1;
    e.description = text.left(16);
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
    e.amount = 50 + QRandomGenerator::global()->bounded(5000);
    e.date = dates[QRandomGenerator::global()->bounded(dates.size())];
    e.project = projects[QRandomGenerator::global()->bounded(projects.size())];
    e.papersRelated = QRandomGenerator::global()->bounded(5);
    e.reimbursable = QRandomGenerator::global()->bounded(2) == 0;
    e.receipt = receipts[QRandomGenerator::global()->bounded(receipts.size())];
    e.color = e.reimbursable ? QColor(16,185,129) : QColor(239,68,68);
    addEntry(e);
    inputField_->clear();
}

void PaperExpenseLogger::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Log research expenses");
    update();
}

void PaperExpenseLogger::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Log research expenses");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Expense Logger");

    int w = width(), h = height();
    drawExpenseList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperExpenseLogger::drawExpenseList(QPainter& p, const QRect& rect) {
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
                   e.description.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.date + " | " + e.receipt);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(static_cast<int>(e.amount)));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.project + " | " + QString::number(e.papersRelated) + " papers");
    }
}

void PaperExpenseLogger::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"publishing", "equipment", "travel", "software", "services"};
    QString labels[] = {"Publish", "Equip", "Travel", "Software", "Service"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperExpenseLogger::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Expenses", QString::number(entries_.size()), QColor(59,130,246)},
        {"Reimbursable", QString::number(reimbursableCount()), QColor(16,185,129)},
        {"Total", "$" + QString::number(static_cast<int>(totalAmount())), QColor(245,158,11)},
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

void PaperExpenseLogger::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Log research expenses"); return; }
    infoLabel_->setText(QString("%1 expenses | %2 reimbursable | $%3 total")
        .arg(entries_.size()).arg(reimbursableCount()).arg(static_cast<int>(totalAmount())));
}

void PaperExpenseLogger::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ExpenseEntry e;
        e.id = settings_.value("id").toInt();
        e.description = settings_.value("description").toString();
        e.category = settings_.value("category").toString();
        e.amount = settings_.value("amount").toDouble();
        e.date = settings_.value("date").toString();
        e.project = settings_.value("project").toString();
        e.papersRelated = settings_.value("papersRelated").toInt();
        e.reimbursable = settings_.value("reimbursable").toBool();
        e.receipt = settings_.value("receipt").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperExpenseLogger::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("description", entries_[i].description);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("amount", entries_[i].amount);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("project", entries_[i].project);
        settings_.setValue("papersRelated", entries_[i].papersRelated);
        settings_.setValue("reimbursable", entries_[i].reimbursable);
        settings_.setValue("receipt", entries_[i].receipt);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
