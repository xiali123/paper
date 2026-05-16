#include "workspace/PaperResourcePool2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperResourcePool2::PaperResourcePool2(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperResourcePool2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left side: controls
    auto* leftLayout = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "GPU", "CPU", "Storage", "API", "License"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Resource name...");

    trackBtn_ = new QPushButton("Track", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Resources: 0 | Overloaded: 0 | Avg Util: 0.0%", this);

    leftLayout->addWidget(categoryCombo_);
    leftLayout->addWidget(inputField_);
    leftLayout->addWidget(trackBtn_);
    leftLayout->addWidget(clearBtn_);
    leftLayout->addWidget(infoLabel_);

    mainLayout->addLayout(leftLayout);
    mainLayout->addStretch();

    connect(trackBtn_, &QPushButton::clicked, this, &PaperResourcePool2::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperResourcePool2::onClear);
}

void PaperResourcePool2::addEntry(const ResourcePoolEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ResourcePoolEntry> PaperResourcePool2::entries() const {
    return entries_;
}

int PaperResourcePool2::overloadedCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.overloaded) ++count;
    }
    return count;
}

qreal PaperResourcePool2::avgUtilization() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.utilization;
    return sum / entries_.size();
}

QMap<QString, int> PaperResourcePool2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperResourcePool2::onTrack() {
    ResourcePoolEntry entry;
    entry.id = entries_.size() + 1;
    entry.resource = inputField_->text().trimmed();
    if (entry.resource.isEmpty())
        entry.resource = QString("Res_%1").arg(entry.id);

    entry.category = categoryCombo_->currentText();
    if (entry.category == "All")
        entry.category = QString("Cat_%1").arg(entry.id);

    entry.utilization = QRandomGenerator::global()->generateDouble();
    entry.capacity = QRandomGenerator::global()->bounded(10, 1001);
    entry.overloaded = entry.utilization > 0.9;

    QStringList statuses = {"active", "idle", "maintenance", "offline"};
    entry.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];

    QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    entry.color = palette[entry.id % palette.size()];

    entries_.append(entry);
    updateInfo();
    saveSettings();
    emit resourceTracked(entry.id, entry.utilization);
    update();
}

void PaperResourcePool2::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
    repaint();
}

void PaperResourcePool2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));

    int colW = w / 3;
    drawPoolView(p, QRect(10, 50, colW - 20, h - 60));
    drawCategoryChart(p, QRect(colW + 10, 50, colW - 20, h - 60));
    drawStats(p, QRect(2 * colW + 10, 50, colW - 20, h - 60));
}

void PaperResourcePool2::drawPoolView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Resource Pool");

    int y = rect.top() + 24;
    int maxEntries = qMin(entries_.size(), 10);
    int barMaxW = rect.width() - 120;

    for (int i = 0; i < maxEntries; ++i) {
        const auto& e = entries_[i];

        // Background bar
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xe2e8f0));
        p.drawRoundedRect(rect.left(), y, barMaxW, 14, 3, 3);

        // Utilization bar - color by state
        int barW = static_cast<int>(e.utilization * barMaxW);
        if (e.overloaded) {
            p.setBrush(QColor(0xdc2626));
        } else if (e.status == "idle") {
            p.setBrush(QColor(0x94a3b8));
        } else {
            p.setBrush(QColor(0x16a34a));
        }
        p.drawRoundedRect(rect.left(), y, barW, 14, 3, 3);

        // Label
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        p.drawText(rect.left() + barMaxW + 6, y + 11,
            QString("%1 | %2%").arg(e.resource.left(8))
                .arg(static_cast<int>(e.utilization * 100)));

        y += 20;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperResourcePool2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Categories");

    auto counts = categoryCounts();
    int y = rect.top() + 24;
    QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    int ci = 0;
    int barMaxW = rect.width() - 80;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        int barW = qMin(it.value() * 24, barMaxW);
        p.setPen(Qt::NoPen);
        p.setBrush(palette[ci++ % palette.size()]);
        p.drawRoundedRect(rect.left(), y, barW, 16, 3, 3);

        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        p.drawText(rect.left() + barW + 6, y + 13,
            QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperResourcePool2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int y = rect.top() + 24;
    p.setFont(QFont("Sans", 9));

    p.drawText(rect.left(), y, QString("Total Resources: %1").arg(entries_.size()));
    y += 18;

    p.drawText(rect.left(), y, QString("Overloaded: %1").arg(overloadedCount()));
    y += 18;

    p.drawText(rect.left(), y,
        QString("Avg Utilization: %1%")
            .arg(QString::number(avgUtilization() * 100, 'f', 1)));
}

void PaperResourcePool2::updateInfo() {
    infoLabel_->setText(
        QString("Resources: %1 | Overloaded: %2 | Avg Util: %3%")
            .arg(entries_.size())
            .arg(overloadedCount())
            .arg(QString::number(avgUtilization() * 100, 'f', 1)));
}

void PaperResourcePool2::loadSettings() {
    settings_.beginGroup("ResourcePool2");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ResourcePoolEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.resource = settings_.value(QString("resource_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.status = settings_.value(QString("status_%1").arg(i)).toString();
        e.utilization = settings_.value(QString("utilization_%1").arg(i)).toDouble();
        e.capacity = settings_.value(QString("capacity_%1").arg(i)).toInt();
        e.overloaded = settings_.value(QString("overloaded_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperResourcePool2::saveSettings() {
    settings_.beginGroup("ResourcePool2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("resource_%1").arg(i), e.resource);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("status_%1").arg(i), e.status);
        settings_.setValue(QString("utilization_%1").arg(i), e.utilization);
        settings_.setValue(QString("capacity_%1").arg(i), e.capacity);
        settings_.setValue(QString("overloaded_%1").arg(i), e.overloaded);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
