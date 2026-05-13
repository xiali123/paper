#include "tools/PaperPingMonitor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperPingMonitor::PaperPingMonitor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PingMonitor")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        struct Seed { QString host; QString category; QString protocol; };
        Seed seeds[] = {
            {"api.paper-service.io",   "Production",  "HTTPS"},
            {"db.primary.internal",    "Production",  "TCP"},
            {"cdn.static-assets.net",  "Production",  "ICMP"},
            {"staging.api.test",       "Staging",     "HTTPS"},
            {"staging-db.internal",    "Staging",     "TCP"},
            {"dev.local.host",         "Development", "HTTP"},
            {"mock-server.dev",        "Development", "HTTP"},
            {"test-runner.ci",         "Testing",     "SSH"},
        };

        QColor catColors[] = {
            QColor(59, 130, 246),  // Production  #3b82f6
            QColor(22, 163, 74),   // Staging     #16a34a
            QColor(217, 119, 6),   // Development #d97706
            QColor(220, 38, 38),   // Testing     #dc2626
            QColor(124, 58, 237),  // External    #7c3aed
        };
        QMap<QString, int> colorMap = {
            {"Production", 0}, {"Staging", 1}, {"Development", 2},
            {"Testing", 3}, {"External", 4}
        };

        auto* rng = QRandomGenerator::global();
        for (int i = 0; i < 8; ++i) {
            PingMonitorEntry e;
            e.id = i + 1;
            e.host = seeds[i].host;
            e.category = seeds[i].category;
            e.protocol = seeds[i].protocol;
            e.latency = rng->bounded(5, 300);
            e.hops = rng->bounded(1, 15);
            e.reachable = rng->bounded(100) < 85;
            e.color = catColors[colorMap.value(e.category, 4)];
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperPingMonitor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Category:"));

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Production", "Staging", "Development", "Testing", "External"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { update(); });
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter host to ping...");
    toolbar->addWidget(inputField_, 2);

    pingBtn_ = new QPushButton("Ping");
    pingBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; border-radius: 4px; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(pingBtn_, &QPushButton::clicked, this, &PaperPingMonitor::onPing);
    toolbar->addWidget(pingBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPingMonitor::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Ping monitor ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 480);
}

void PaperPingMonitor::addEntry(const PingMonitorEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit hostChecked(entry.id, entry.latency);
    update();
}

QList<PingMonitorEntry> PaperPingMonitor::entries() const {
    return entries_;
}

int PaperPingMonitor::reachableCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.reachable) c++;
    return c;
}

qreal PaperPingMonitor::avgLatency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_)
        sum += e.latency;
    return sum / entries_.size();
}

QMap<QString, int> PaperPingMonitor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperPingMonitor::onPing() {
    QString host = inputField_->text().trimmed();
    if (host.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    if (category == "All") category = "External";

    QColor catColors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };
    QMap<QString, int> colorMap = {
        {"Production", 0}, {"Staging", 1}, {"Development", 2},
        {"Testing", 3}, {"External", 4}
    };

    auto* rng = QRandomGenerator::global();

    PingMonitorEntry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.host = host;
    e.category = category;
    e.protocol = (QStringList{"HTTPS","TCP","ICMP","HTTP","SSH"}).at(rng->bounded(5));
    e.latency = rng->bounded(5, 500);
    e.hops = rng->bounded(1, 20);
    e.reachable = rng->bounded(100) < 80;
    e.color = catColors[colorMap.value(category, 4)];

    addEntry(e);
    inputField_->clear();
}

void PaperPingMonitor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Ping monitor ready");
    update();
}

void PaperPingMonitor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No ping entries — click Ping to start");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Ping Monitor");

    int w = width(), h = height();
    int halfW = w / 2 - 25;
    drawMonitorView(p, QRect(20, 50, halfW, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, halfW, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, halfW, h / 2 - 50));
}

void PaperPingMonitor::drawMonitorView(QPainter& p, const QRect& rect) {
    QString filter = categoryCombo_->currentText();

    QList<const PingMonitorEntry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter)
            visible.append(&e);
    }

    int maxShow = qMin(10, visible.size());
    int itemH = qMin(42, (rect.height() - 10) / qMax(maxShow, 1));
    if (itemH < 20) itemH = 20;

    int maxLatency = 1;
    for (const auto* e : visible)
        maxLatency = qMax(maxLatency, static_cast<int>(e->latency));

    for (int i = 0; i < maxShow; ++i) {
        const auto* e = visible[visible.size() - 1 - i];
        int y = rect.y() + i * (itemH + 3);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e->reachable ? QColor(248, 250, 252) : QColor(254, 242, 242));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Color indicator bar on left
        p.setBrush(e->color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Reachability dot
        p.setBrush(e->reachable ? QColor(34, 197, 94) : QColor(239, 68, 68));
        p.drawEllipse(rect.x() + 12, y + itemH / 2 - 4, 8, 8);

        // Host name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 26, y + 2, rect.width() * 0.5, 16, Qt::AlignVCenter,
                   e->host.left(28));

        // Protocol + Hops
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 26, y + 18, rect.width() * 0.5, 14, Qt::AlignVCenter,
                   e->protocol + " | Hops: " + QString::number(e->hops));

        // Latency bar
        int barX = rect.x() + rect.width() * 0.55;
        int barMaxW = rect.width() * 0.35;
        int barW = static_cast<int>((e->latency / maxLatency) * barMaxW);
        barW = qMax(barW, 2);

        QColor barColor = e->latency < 50 ? QColor(34, 197, 94)
                        : e->latency < 150 ? QColor(245, 158, 11)
                        : QColor(239, 68, 68);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor.lighter(170));
        p.drawRoundedRect(barX, y + 4, barW, itemH - 14, 3, 3);
        p.setBrush(barColor);
        p.drawRoundedRect(barX, y + 4, qMin(barW, static_cast<int>(barMaxW)), itemH - 14, 3, 3);

        // Latency value
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(barX + barW + 4, y + itemH / 2 - 2,
                   QString::number(static_cast<int>(e->latency)) + "ms");
    }
}

void PaperPingMonitor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList categories = {"Production", "Staging", "Development", "Testing", "External"};
    QColor catColors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int total = 0;
    for (const auto& cat : categories)
        total += counts.value(cat, 0);

    if (total == 0) return;

    // Donut chart
    int side = qMin(rect.width(), rect.height() - 30) - 20;
    int cx = rect.x() + rect.width() / 2 - side / 2;
    int cy = rect.y() + 30;
    int outerR = side / 2;
    int innerR = outerR * 55 / 100;

    int startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.value(categories[i], 0);
        if (count == 0) continue;
        int spanAngle = static_cast<int>(360.0 * count / total * 16);

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawPie(cx, cy, side, side, -startAngle * 16, -spanAngle);
        startAngle += spanAngle / 16;
    }

    // Inner circle to create donut
    p.setBrush(Qt::white);
    p.drawEllipse(cx + outerR - innerR, cy + outerR - innerR,
                  innerR * 2, innerR * 2);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(QRect(cx, cy + outerR - innerR, side, innerR * 2),
               Qt::AlignCenter, QString::number(total));
    p.setFont(QFont("Arial", 8));
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(cx, cy + outerR + 4, side, 20),
               Qt::AlignHCenter | Qt::AlignTop, "hosts");
}

void PaperPingMonitor::drawStats(QPainter& p, const QRect& rect) {
    int totalHops = 0;
    for (const auto& e : entries_)
        totalHops += e.hops;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total",        QString::number(entries_.size()),   QColor(59, 130, 246)},
        {"Reachable",    QString::number(reachableCount()),  QColor(22, 163, 74)},
        {"Avg Latency",  QString::number(static_cast<int>(avgLatency())) + "ms",
                                                         QColor(217, 119, 6)},
        {"Total Hops",   QString::number(totalHops),         QColor(124, 58, 237)}
    };

    int cols = 2, rows = 2;
    int boxW = (rect.width() - 10) / cols;
    int boxH = (rect.height() - 10) / rows;

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int x = rect.x() + col * (boxW + 5);
        int y = rect.y() + row * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, y + 5, boxW - 20, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + boxH / 2, boxW - 20, boxH / 2, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperPingMonitor::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Ping monitor ready");
        return;
    }
    infoLabel_->setText(QString("%1 hosts | %2 reachable | Avg %3ms")
        .arg(entries_.size())
        .arg(reachableCount())
        .arg(static_cast<int>(avgLatency())));
}

void PaperPingMonitor::loadSettings() {
    int size = settings_.beginReadArray("pingEntries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PingMonitorEntry e;
        e.id = settings_.value("id").toInt();
        e.host = settings_.value("host").toString();
        e.category = settings_.value("category").toString();
        e.protocol = settings_.value("protocol").toString();
        e.latency = settings_.value("latency").toReal();
        e.hops = settings_.value("hops").toInt();
        e.reachable = settings_.value("reachable").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
}

void PaperPingMonitor::saveSettings() {
    settings_.beginWriteArray("pingEntries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("host", entries_[i].host);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("protocol", entries_[i].protocol);
        settings_.setValue("latency", entries_[i].latency);
        settings_.setValue("hops", entries_[i].hops);
        settings_.setValue("reachable", entries_[i].reachable);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
