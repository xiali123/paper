#include "tools/PaperServiceMonitor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperServiceMonitor::PaperServiceMonitor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ServiceMonitor")
{
    setupUI();
    loadSettings();
}

void PaperServiceMonitor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperServiceMonitor::onCheck);
    toolbar->addWidget(checkBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperServiceMonitor::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "API", "Database", "Cache", "Queue", "Storage"});
    toolbar->addWidget(categoryCombo_, 1);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter service name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Monitor service status");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperServiceMonitor::addEntry(const ServiceEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit serviceChecked(entry.id, entry.latency);
    update();
}

QList<ServiceEntry> PaperServiceMonitor::entries() const { return entries_; }

int PaperServiceMonitor::healthyCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.healthy) c++;
    return c;
}

qreal PaperServiceMonitor::avgLatency() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.latency;
    return sum / entries_.size();
}

QMap<QString, int> PaperServiceMonitor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperServiceMonitor::onCheck() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"API", "Database", "Cache", "Queue", "Storage"};
    QStringList statuses = {"running", "degraded", "stopped", "maintenance"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int catIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        ServiceEntry e;
        e.id = entries_.size() + 1;
        e.name = text + "-" + QString::number(i + 1);
        int cIdx = catIdx == 0 ? QRandomGenerator::global()->bounded(categories.size()) : catIdx - 1;
        e.category = categories[cIdx];
        e.latency = 5.0 + QRandomGenerator::global()->bounded(500);
        e.uptime = 95.0 + QRandomGenerator::global()->bounded(50) / 10.0;
        e.errors = QRandomGenerator::global()->bounded(20);
        e.status = e.latency < 100 ? "running" : (e.latency < 300 ? "degraded" : "stopped");
        e.healthy = e.uptime > 99.0 && e.errors < 5;
        e.color = palette[cIdx % 5];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperServiceMonitor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Monitor service status");
    update();
}

void PaperServiceMonitor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Monitor service status");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Service Monitor");
    int w = width(), h = height();
    drawServiceList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperServiceMonitor::drawServiceList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.name.left(14) + (e.healthy ? " [OK]" : " [!!]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.uptime, 'f', 1) + "% up");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.latency, 'f', 0) + "ms");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.errors) + " err | " + e.status);
    }
}

void PaperServiceMonitor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"API", "Database", "Cache", "Queue", "Storage"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperServiceMonitor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Services", QString::number(entries_.size()), QColor(59,130,246)},
        {"Healthy", QString::number(healthyCount()), QColor(22,163,106)},
        {"Avg Latency", QString::number(avgLatency(), 'f', 0) + "ms", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperServiceMonitor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Monitor service status"); return; }
    infoLabel_->setText(QString("%1 services | %2 healthy | %3ms avg latency")
        .arg(entries_.size()).arg(healthyCount()).arg(avgLatency(), 0, 'f', 0));
}

void PaperServiceMonitor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ServiceEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.uptime = settings_.value("uptime").toDouble();
        e.latency = settings_.value("latency").toDouble();
        e.errors = settings_.value("errors").toInt();
        e.healthy = settings_.value("healthy").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperServiceMonitor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("uptime", entries_[i].uptime);
        settings_.setValue("latency", entries_[i].latency);
        settings_.setValue("errors", entries_[i].errors);
        settings_.setValue("healthy", entries_[i].healthy);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
