#include "workspace/PaperContractManager2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperContractManager2::PaperContractManager2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContractManager2")
{
    setupUI();
    loadSettings();
}

void PaperContractManager2::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* left = new QHBoxLayout();
    left->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Software", "Hardware", "Service", "Consulting", "License"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Vendor name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(inputField_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperContractManager2::onAdd);
    left->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContractManager2::onClear);
    left->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Contracts: 0 | Active: 0 | Total: $0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    left->addWidget(infoLabel_);

    layout->addLayout(left);
    layout->addStretch();

    setMinimumSize(640, 480);
}

void PaperContractManager2::addEntry(const ContractEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contractAdded(entry.id, entry.value);
    update();
}

QList<ContractEntry> PaperContractManager2::entries() const { return entries_; }

int PaperContractManager2::activeCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.active) ++c;
    return c;
}

qreal PaperContractManager2::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

QMap<QString, int> PaperContractManager2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperContractManager2::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Software", "Hardware", "Service", "Consulting", "License"};
    QStringList statuses = {"active", "expired", "pending"};

    int catIdx = categoryCombo_->currentIndex();
    ContractEntry e;
    e.id = entries_.size() + 1;
    e.vendor = text;
    e.category = catIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                              : categories[catIdx - 1];
    e.value = 1000 + QRandomGenerator::global()->bounded(100000);
    e.daysLeft = 1 + QRandomGenerator::global()->bounded(365);
    e.active = QRandomGenerator::global()->bounded(2) == 0;
    int sIdx = QRandomGenerator::global()->bounded(statuses.size());
    e.status = statuses[sIdx];

    if (e.status == "active")
        e.color = QColor(22, 163, 74);   // #16a34a green
    else if (e.status == "expired")
        e.color = QColor(220, 38, 38);   // #dc2626 red
    else
        e.color = QColor(217, 119, 6);   // #d97706 amber

    addEntry(e);
    inputField_->clear();
}

void PaperContractManager2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperContractManager2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No contracts yet");
        return;
    }

    int w = width(), h = height();
    int colW = w / 3;

    drawContractList(p, QRect(10, 10, colW - 15, h - 20));
    drawCategoryChart(p, QRect(colW + 5, 10, colW - 15, h - 20));
    drawStats(p, QRect(2 * colW + 5, 10, colW - 15, h - 20));
}

void PaperContractManager2::drawContractList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Contract Manager");

    int show = qMin(12, entries_.size());
    qreal maxVal = 1;
    for (int i = 0; i < show; ++i)
        maxVal = qMax(maxVal, entries_[i].value);

    int itemH = qMin(32, (rect.height() - 35) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 30 + i * (itemH + 3);

        // Background bar proportional to value
        int barW = static_cast<int>((e.value / maxVal) * (rect.width() - 8));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x() + 4, y, barW, itemH, 4, 4);

        // Status indicator stripe
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 4, y, 4, itemH, 2, 2);

        // Vendor name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 12, y + 2, rect.width() / 2, 16, Qt::AlignVCenter,
                   e.vendor.left(16));

        // Value and status
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 12, y + 17, rect.width() / 2, 14, Qt::AlignVCenter,
                   "$" + QString::number(static_cast<int>(e.value)) + " | " + e.status);

        // Days left
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 12, itemH - 6,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.daysLeft) + "d left");
    }
}

void PaperContractManager2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Software", "Hardware", "Service", "Consulting", "License"};
    QColor catColors[] = {
        QColor(59, 130, 246),   // #3b82f6 blue
        QColor(124, 58, 237),   // #7c3aed purple
        QColor(22, 163, 74),    // #16a34a green
        QColor(217, 119, 6),    // #d97706 amber
        QColor(220, 38, 38)     // #dc2626 red
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(26, (rect.height() - 40) / categories.size());

    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 32 + i * (barH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter,
                   categories[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperContractManager2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Contracts", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Active Count",    QString::number(activeCount()),   QColor(22, 163, 74)},
        {"Total Value",     "$" + QString::number(static_cast<int>(totalValue())),
                                                             QColor(217, 119, 6)}
    };

    int boxH = qMin(60, (rect.height() - 20) / stats.size());

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 6);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Left accent stripe
        p.setBrush(stats[i].color);
        p.drawRoundedRect(rect.x(), y, 4, boxH, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 12, y + 6, rect.width() - 20, 28, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 34, rect.width() - 20, 18, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperContractManager2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Contracts: 0 | Active: 0 | Total: $0");
        return;
    }
    infoLabel_->setText(QString("Contracts: %1 | Active: %2 | Total: $%3")
        .arg(entries_.size())
        .arg(activeCount())
        .arg(static_cast<int>(totalValue())));
}

void PaperContractManager2::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContractEntry e;
        e.id       = settings_.value("id").toInt();
        e.vendor   = settings_.value("vendor").toString();
        e.category = settings_.value("category").toString();
        e.status   = settings_.value("status").toString();
        e.value    = settings_.value("value").toDouble();
        e.daysLeft = settings_.value("daysLeft").toInt();
        e.active   = settings_.value("active").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContractManager2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("vendor",   entries_[i].vendor);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status",   entries_[i].status);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("daysLeft", entries_[i].daysLeft);
        settings_.setValue("active",   entries_[i].active);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
