#include "workspace/PaperResourceScheduler.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperResourceScheduler::PaperResourceScheduler(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ResourceScheduler")
{
    setupUI();
    loadSettings();
}

void PaperResourceScheduler::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    scheduleBtn_ = new QPushButton("Schedule");
    scheduleBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scheduleBtn_, &QPushButton::clicked, this, &PaperResourceScheduler::onSchedule);
    toolbar->addWidget(scheduleBtn_);
    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"All", "GPU", "CPU", "Storage", "Network"});
    toolbar->addWidget(typeCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperResourceScheduler::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter resource name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Schedule resources");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperResourceScheduler::addEntry(const ResourceEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit resourceScheduled(entry.id, entry.utilization);
    update();
}

QList<ResourceEntry> PaperResourceScheduler::entries() const { return entries_; }

qreal PaperResourceScheduler::avgUtilization() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.utilization;
    return sum / entries_.size();
}

int PaperResourceScheduler::availableCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.available) c++;
    return c;
}

QMap<QString, int> PaperResourceScheduler::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.type]++;
    return counts;
}

void PaperResourceScheduler::onSchedule() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList types = {"gpu", "cpu", "storage", "network"};
    QStringList schedules = {"daily", "weekly", "on-demand", "reserved"};
    int tIdx = typeCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ResourceEntry e;
        e.id = entries_.size() + 1;
        e.resourceName = text.left(8) + " res" + QString::number(i);
        e.type = tIdx == 0 ? types[QRandomGenerator::global()->bounded(types.size())] : types[tIdx - 1];
        e.capacity = 10 + QRandomGenerator::global()->bounded(100);
        e.allocated = QRandomGenerator::global()->bounded(e.capacity + 1);
        e.schedule = schedules[QRandomGenerator::global()->bounded(schedules.size())];
        e.utilization = static_cast<qreal>(e.allocated) / e.capacity;
        e.available = e.utilization < 0.9;
        e.color = e.available ? (e.utilization > 0.7 ? QColor(245,158,11) : QColor(16,185,129)) : QColor(239,68,68);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperResourceScheduler::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Schedule resources");
    update();
}

void PaperResourceScheduler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Schedule resources");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Resource Scheduler");
    int w = width(), h = height();
    drawResourceList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperResourceScheduler::drawResourceList(QPainter& p, const QRect& rect) {
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
                   e.resourceName.left(14) + (e.available ? "" : " [FULL]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.type + " | " + e.schedule + " | " + QString::number(e.allocated) + "/" + QString::number(e.capacity));
        int barW = static_cast<int>(e.utilization * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22,
                   QString::number(e.utilization * 100, 'f', 0) + "%");
    }
}

void PaperResourceScheduler::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Types");
    auto counts = typeCounts();
    QStringList types = {"gpu", "cpu", "storage", "network"};
    QString labels[] = {"GPU", "CPU", "Storage", "Network"};
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

void PaperResourceScheduler::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Resources", QString::number(entries_.size()), QColor(59,130,246)},
        {"Available", QString::number(availableCount()), QColor(16,185,129)},
        {"Avg Util", QString::number(avgUtilization() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperResourceScheduler::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Schedule resources"); return; }
    infoLabel_->setText(QString("%1 resources | %2 available | %3% util")
        .arg(entries_.size()).arg(availableCount()).arg(avgUtilization() * 100, 0, 'f', 0));
}

void PaperResourceScheduler::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ResourceEntry e;
        e.id = settings_.value("id").toInt();
        e.resourceName = settings_.value("resourceName").toString();
        e.type = settings_.value("type").toString();
        e.capacity = settings_.value("capacity").toInt();
        e.allocated = settings_.value("allocated").toInt();
        e.schedule = settings_.value("schedule").toString();
        e.utilization = settings_.value("utilization").toDouble();
        e.available = settings_.value("available").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperResourceScheduler::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("resourceName", entries_[i].resourceName);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("capacity", entries_[i].capacity);
        settings_.setValue("allocated", entries_[i].allocated);
        settings_.setValue("schedule", entries_[i].schedule);
        settings_.setValue("utilization", entries_[i].utilization);
        settings_.setValue("available", entries_[i].available);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
