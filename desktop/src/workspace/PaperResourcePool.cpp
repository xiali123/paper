#include "workspace/PaperResourcePool.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperResourcePool::PaperResourcePool(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperResourcePool::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "GPU", "CPU", "Storage", "API", "License"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Resource name...");
    allocateBtn_ = new QPushButton("Allocate", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Resources: 0 | Available: 0 | Avg Util: 0%", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(allocateBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(allocateBtn_, &QPushButton::clicked, this, &PaperResourcePool::onAllocate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperResourcePool::onClear);
}

void PaperResourcePool::addEntry(const ResourceEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ResourceEntry> PaperResourcePool::entries() const { return entries_; }

int PaperResourcePool::availableCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.available) c++;
    return c;
}

qreal PaperResourcePool::avgUtilization() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.utilization;
    return sum / entries_.size();
}

QMap<QString, int> PaperResourcePool::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperResourcePool::onAllocate() {
    ResourceEntry e;
    e.id = entries_.size() + 1;
    e.resource = inputField_->text().trimmed();
    if (e.resource.isEmpty()) e.resource = QString("Res_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList types = {"Compute", "Memory", "Network", "Disk", "Token"};
    e.type = types[QRandomGenerator::global()->bounded(types.size())];
    e.allocated = QRandomGenerator::global()->bounded(10, 1000);
    e.used = QRandomGenerator::global()->bounded(0, e.allocated);
    e.utilization = static_cast<qreal>(e.used) / e.allocated;
    e.available = e.utilization < 0.8;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit resourceAllocated(e.id, e.utilization);
    update();
}

void PaperResourcePool::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperResourcePool::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawResourceList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperResourcePool::drawResourceList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Resource Pool:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        // Utilization bar
        int barW = static_cast<int>(e.utilization * 80);
        p.setBrush(QColor(0xe2e8f0));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y + 2, 80, 10, 2, 2);
        QColor barColor = e.available ? QColor(0x16a34a) : QColor(0xdc2626);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.left(), y + 2, barW, 10, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 86, y + 11, QString("%1 | %2 | %3/%4 | %5%")
            .arg(e.resource.left(10), e.type)
            .arg(e.used).arg(e.allocated)
            .arg(static_cast<int>(e.utilization * 100)));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperResourcePool::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperResourcePool::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Available: %1").arg(availableCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Utilization: %1%").arg(static_cast<int>(avgUtilization() * 100)));
}

void PaperResourcePool::updateInfo() {
    infoLabel_->setText(QString("Resources: %1 | Available: %2 | Avg Util: %3%")
        .arg(entries_.size()).arg(availableCount())
        .arg(static_cast<int>(avgUtilization() * 100)));
}

void PaperResourcePool::loadSettings() {
    settings_.beginGroup("ResourcePool");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ResourceEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.resource = settings_.value(QString("resource_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.type = settings_.value(QString("type_%1").arg(i)).toString();
        e.allocated = settings_.value(QString("allocated_%1").arg(i)).toInt();
        e.used = settings_.value(QString("used_%1").arg(i)).toInt();
        e.utilization = settings_.value(QString("utilization_%1").arg(i)).toDouble();
        e.available = settings_.value(QString("available_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperResourcePool::saveSettings() {
    settings_.beginGroup("ResourcePool");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("resource_%1").arg(i), e.resource);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("type_%1").arg(i), e.type);
        settings_.setValue(QString("allocated_%1").arg(i), e.allocated);
        settings_.setValue(QString("used_%1").arg(i), e.used);
        settings_.setValue(QString("utilization_%1").arg(i), e.utilization);
        settings_.setValue(QString("available_%1").arg(i), e.available);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
