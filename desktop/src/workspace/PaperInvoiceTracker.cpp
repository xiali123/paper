#include "workspace/PaperInvoiceTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperInvoiceTracker::PaperInvoiceTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "InvoiceTracker")
{
    setupUI();
    loadSettings();
}

void PaperInvoiceTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Invoice");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperInvoiceTracker::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Publishing", "Equipment", "Travel", "Software"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperInvoiceTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter vendor name or invoice reference...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track research invoices");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperInvoiceTracker::addEntry(const InvoiceEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit invoiceTracked(entry.id, entry.balance);
    update();
}

QList<InvoiceEntry> PaperInvoiceTracker::entries() const { return entries_; }

qreal PaperInvoiceTracker::totalAmount() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.amount;
    return t;
}

int PaperInvoiceTracker::overdueCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.overdue) c++;
    return c;
}

QMap<QString, int> PaperInvoiceTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperInvoiceTracker::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"publishing", "equipment", "travel", "software", "services"};
    QStringList statuses = {"paid", "pending", "overdue", "partial"};
    QStringList dueDates = {"2026-05-15", "2026-06-01", "2026-06-15", "2026-07-01"};
    QStringList vendors = {"Publisher A", "Lab Supply Co", "Travel Agency", "Software Inc"};

    int cIdx = categoryCombo_->currentIndex();
    InvoiceEntry e;
    e.id = entries_.size() + 1;
    e.invoiceId = "INV-" + QString::number(1000 + entries_.size());
    e.vendor = text.left(12) + " " + vendors[entries_.size() % vendors.size()];
    e.amount = 200 + QRandomGenerator::global()->bounded(8000);
    int paidAmount = QRandomGenerator::global()->bounded(static_cast<int>(e.amount + 500));
    e.paid = qMin(static_cast<qreal>(paidAmount), e.amount * 1.1);
    e.balance = e.amount - e.paid;
    int sIdx = QRandomGenerator::global()->bounded(statuses.size());
    e.status = statuses[sIdx];
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
    e.dueDate = dueDates[QRandomGenerator::global()->bounded(dueDates.size())];
    e.papersRelated = QRandomGenerator::global()->bounded(8);
    e.overdue = e.status == "overdue" || (e.balance > 0 && QRandomGenerator::global()->bounded(3) == 0);
    e.color = e.overdue ? QColor(239,68,68) : (e.balance <= 0 ? QColor(16,185,129) : QColor(245,158,11));
    addEntry(e);
    inputField_->clear();
}

void PaperInvoiceTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track research invoices");
    update();
}

void PaperInvoiceTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track research invoices");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Invoice Tracker");

    int w = width(), h = height();
    drawInvoiceList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperInvoiceTracker::drawInvoiceList(QPainter& p, const QRect& rect) {
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
                   e.invoiceId + (e.overdue ? " [!]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.vendor.left(16) + " | " + e.dueDate);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(static_cast<int>(e.amount)));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "bal $" + QString::number(static_cast<int>(e.balance)) + " | " + QString::number(e.papersRelated) + " papers");
    }
}

void PaperInvoiceTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperInvoiceTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Invoices", QString::number(entries_.size()), QColor(59,130,246)},
        {"Overdue", QString::number(overdueCount()), QColor(239,68,68)},
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

void PaperInvoiceTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track research invoices"); return; }
    infoLabel_->setText(QString("%1 invoices | %2 overdue | $%3 total")
        .arg(entries_.size()).arg(overdueCount()).arg(static_cast<int>(totalAmount())));
}

void PaperInvoiceTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        InvoiceEntry e;
        e.id = settings_.value("id").toInt();
        e.invoiceId = settings_.value("invoiceId").toString();
        e.vendor = settings_.value("vendor").toString();
        e.amount = settings_.value("amount").toDouble();
        e.paid = settings_.value("paid").toDouble();
        e.balance = settings_.value("balance").toDouble();
        e.status = settings_.value("status").toString();
        e.category = settings_.value("category").toString();
        e.dueDate = settings_.value("dueDate").toString();
        e.papersRelated = settings_.value("papersRelated").toInt();
        e.overdue = settings_.value("overdue").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperInvoiceTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("invoiceId", entries_[i].invoiceId);
        settings_.setValue("vendor", entries_[i].vendor);
        settings_.setValue("amount", entries_[i].amount);
        settings_.setValue("paid", entries_[i].paid);
        settings_.setValue("balance", entries_[i].balance);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("dueDate", entries_[i].dueDate);
        settings_.setValue("papersRelated", entries_[i].papersRelated);
        settings_.setValue("overdue", entries_[i].overdue);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
