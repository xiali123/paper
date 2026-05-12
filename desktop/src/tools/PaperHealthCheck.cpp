#include "tools/PaperHealthCheck.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperHealthCheck::PaperHealthCheck(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HealthCheck")
{
    setupUI();
    loadSettings();
}

void PaperHealthCheck::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperHealthCheck::onCheck);
    toolbar->addWidget(checkBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "database", "api", "storage", "auth", "cache"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHealthCheck::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter service name to check...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Check service health");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperHealthCheck::addEntry(const HealthEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit healthChecked(entry.id, entry.responseTime);
    update();
}

QList<HealthEntry> PaperHealthCheck::entries() const { return entries_; }

int PaperHealthCheck::healthyCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.healthy) c++;
    return c;
}

qreal PaperHealthCheck::avgResponseTime() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.responseTime;
    return sum / entries_.size();
}

QMap<QString, int> PaperHealthCheck::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperHealthCheck::onCheck() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"database", "api", "storage", "auth", "cache"};
    QStringList statuses = {"operational", "degraded", "down", "maintenance", "unknown"};
    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int catIdx = categoryCombo_->currentIndex();
    if (catIdx < 1 || catIdx > categories.size()) catIdx = 1 + QRandomGenerator::global()->bounded(categories.size());
    QString category = (catIdx >= 1 && catIdx <= categories.size()) ? categories[catIdx - 1] : categories[0];

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        HealthEntry e;
        e.id = entries_.size() + 1;
        e.service = text;
        e.category = categories[i % categories.size()];
        e.uptime = 95.0 + QRandomGenerator::global()->bounded(500) / 100.0;
        e.responseTime = 5 + QRandomGenerator::global()->bounded(500);
        e.checks = 10 + QRandomGenerator::global()->bounded(990);
        e.healthy = e.uptime > 99.5 && e.responseTime < 200;

        if (e.healthy) e.status = "operational";
        else if (e.uptime > 98.0) e.status = "degraded";
        else if (e.uptime > 95.0) e.status = "maintenance";
        else e.status = "down";

        e.color = palette[i % 5];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperHealthCheck::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Check service health");
    update();
}

void PaperHealthCheck::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Check service health");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Health Check");

    int w = width(), h = height();
    drawHealthList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperHealthCheck::drawHealthList(QPainter& p, const QRect& rect) {
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
                   e.service.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.checks) + " checks | " + e.status);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.uptime, 'f', 1) + "% up | " + QString::number(e.responseTime, 'f', 0) + "ms");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   (e.healthy ? QString("healthy") : QString("unhealthy")));
    }
}

void PaperHealthCheck::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"database", "api", "storage", "auth", "cache"};
    QString labels[] = {"Database", "API", "Storage", "Auth", "Cache"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperHealthCheck::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Services", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Healthy", QString::number(healthyCount()), QColor(22, 163, 74)},
        {"Avg Time", QString::number(avgResponseTime(), 'f', 0) + "ms", QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
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

void PaperHealthCheck::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Check service health"); return; }
    infoLabel_->setText(QString("%1 services | %2 healthy | %3ms avg")
        .arg(entries_.size()).arg(healthyCount()).arg(avgResponseTime(), 0, 'f', 0));
}

void PaperHealthCheck::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HealthEntry e;
        e.id = settings_.value("id").toInt();
        e.service = settings_.value("service").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.uptime = settings_.value("uptime").toDouble();
        e.responseTime = settings_.value("responseTime").toDouble();
        e.checks = settings_.value("checks").toInt();
        e.healthy = settings_.value("healthy").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHealthCheck::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("service", entries_[i].service);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("uptime", entries_[i].uptime);
        settings_.setValue("responseTime", entries_[i].responseTime);
        settings_.setValue("checks", entries_[i].checks);
        settings_.setValue("healthy", entries_[i].healthy);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
