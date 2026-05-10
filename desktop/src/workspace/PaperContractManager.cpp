#include "workspace/PaperContractManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperContractManager::PaperContractManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContractManager")
{
    setupUI();
    loadSettings();
}

void PaperContractManager::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Contract");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperContractManager::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"All", "License", "Subscription", "Service", "Grant"});
    toolbar->addWidget(typeCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContractManager::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter vendor or contract name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Manage research contracts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperContractManager::addEntry(const ContractEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contractAdded(entry.id, entry.value);
    update();
}

QList<ContractEntry> PaperContractManager::entries() const { return entries_; }

qreal PaperContractManager::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

int PaperContractManager::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

QMap<QString, int> PaperContractManager::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.type]++;
    return counts;
}

void PaperContractManager::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"license", "subscription", "service", "grant"};
    QStringList statuses = {"active", "expired", "pending", "renewed"};
    QStringList vendors = {"Publisher", "Lab Corp", "Software Inc", "University Press"};
    QStringList starts = {"2025-01-01", "2025-06-01", "2026-01-01", "2026-03-01"};
    QStringList ends = {"2026-01-01", "2026-06-01", "2027-01-01", "2027-06-01"};

    int tIdx = typeCombo_->currentIndex();
    ContractEntry e;
    e.id = entries_.size() + 1;
    e.contractId = "CTR-" + QString::number(2000 + entries_.size());
    e.vendor = text.left(12) + " " + vendors[entries_.size() % vendors.size()];
    e.type = tIdx == 0 ? types[QRandomGenerator::global()->bounded(types.size())] : types[tIdx - 1];
    e.value = 1000 + QRandomGenerator::global()->bounded(50000);
    e.startDate = starts[QRandomGenerator::global()->bounded(starts.size())];
    e.endDate = ends[QRandomGenerator::global()->bounded(ends.size())];
    int sIdx = QRandomGenerator::global()->bounded(statuses.size());
    e.status = statuses[sIdx];
    e.papersCovered = QRandomGenerator::global()->bounded(20);
    e.active = e.status == "active" || e.status == "renewed";
    e.color = e.active ? QColor(16,185,129) : (e.status == "pending" ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperContractManager::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Manage research contracts");
    update();
}

void PaperContractManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage research contracts");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Contract Manager");

    int w = width(), h = height();
    drawContractList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperContractManager::drawContractList(QPainter& p, const QRect& rect) {
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
                   e.contractId + (e.active ? " [A]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.vendor.left(14) + " | " + e.startDate + "-" + e.endDate);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(static_cast<int>(e.value)));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.type + " | " + QString::number(e.papersCovered) + " papers");
    }
}

void PaperContractManager::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Types");

    auto counts = typeCounts();
    QStringList types = {"license", "subscription", "service", "grant"};
    QString labels[] = {"License", "Subscript", "Service", "Grant"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

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

void PaperContractManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Contracts", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(16,185,129)},
        {"Total Value", "$" + QString::number(static_cast<int>(totalValue())), QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperContractManager::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Manage research contracts"); return; }
    infoLabel_->setText(QString("%1 contracts | %2 active | $%3 total")
        .arg(entries_.size()).arg(activeCount()).arg(static_cast<int>(totalValue())));
}

void PaperContractManager::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContractEntry e;
        e.id = settings_.value("id").toInt();
        e.contractId = settings_.value("contractId").toString();
        e.vendor = settings_.value("vendor").toString();
        e.type = settings_.value("type").toString();
        e.value = settings_.value("value").toDouble();
        e.startDate = settings_.value("startDate").toString();
        e.endDate = settings_.value("endDate").toString();
        e.status = settings_.value("status").toString();
        e.papersCovered = settings_.value("papersCovered").toInt();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContractManager::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("contractId", entries_[i].contractId);
        settings_.setValue("vendor", entries_[i].vendor);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("startDate", entries_[i].startDate);
        settings_.setValue("endDate", entries_[i].endDate);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("papersCovered", entries_[i].papersCovered);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
