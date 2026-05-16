#include "tools/PaperLoadBalancer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLoadBalancer::PaperLoadBalancer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LoadBalancer")
{
    setupUI();
    loadSettings();
}

void PaperLoadBalancer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperLoadBalancer::onCheck);
    toolbar->addWidget(checkBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Web", "API", "Database", "Cache", "Queue"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Server address...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLoadBalancer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Load Balancer Monitor");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperLoadBalancer::addEntry(const LoadBalancerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit serverChecked(entry.id, entry.load);
    update();
}

QList<LoadBalancerEntry> PaperLoadBalancer::entries() const { return entries_; }

int PaperLoadBalancer::healthyCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.healthy) c++;
    return c;
}

qreal PaperLoadBalancer::avgLoad() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.load;
    return sum / entries_.size();
}

QMap<QString, int> PaperLoadBalancer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLoadBalancer::onCheck() {
    QStringList servers = {"nginx-01", "haproxy-02", "traefik-03", "envoy-04",
                          "caddy-05",  "varnish-06",  "squid-07",   "ats-08"};
    QStringList categories = {"Web", "API", "Database", "Cache", "Queue"};
    QStringList algorithms = {"RoundRobin", "LeastConn", "Weighted", "IPHash", "Random"};
    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    QString server = inputField_->text().trimmed();
    if (server.isEmpty()) server = servers[QRandomGenerator::global()->bounded(servers.size())];

    for (int i = 0; i < 8; ++i) {
        LoadBalancerEntry e;
        e.id = entries_.size() + 1;
        e.server = (i == 0) ? server : servers[QRandomGenerator::global()->bounded(servers.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.algorithm = algorithms[QRandomGenerator::global()->bounded(algorithms.size())];
        e.load = QRandomGenerator::global()->bounded(100) / 100.0;
        e.connections = QRandomGenerator::global()->bounded(500) + 10;
        e.healthy = e.load < 0.85;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperLoadBalancer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Load Balancer Monitor");
    update();
}

void PaperLoadBalancer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Load Balancer Monitor");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Load Balancer");

    int w = width(), h = height();
    drawBalancerView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLoadBalancer::drawBalancerView(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx > 0) {
            QStringList cats = {"All", "Web", "API", "Database", "Cache", "Queue"};
            if (e.category != cats[filterIdx]) continue;
        }

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.healthy ? Qt::white : e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Health indicator dot
        p.setPen(Qt::NoPen);
        p.setBrush(e.healthy ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.drawEllipse(rect.x() + 6, y + itemH / 2 - 4, 8, 8);

        // Server name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 20, y + 2, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.server.left(20));

        // Category + algorithm
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 20, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.algorithm);

        // Load bar background
        int barX = rect.x() + rect.width() / 2 + 10;
        int barW = rect.width() / 2 - 60;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, y + 4, barW, 10, 3, 3);

        // Load bar fill
        QColor barColor = e.load > 0.85 ? QColor(220, 38, 38)
                        : e.load > 0.6  ? QColor(217, 119, 6)
                        :                  QColor(22, 163, 74);
        p.setBrush(barColor);
        p.drawRoundedRect(barX, y + 4, static_cast<int>(barW * qMin(e.load, 1.0)), 10, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(barX + barW + 4, y + 14,
                   QString("%1%").arg(e.load * 100, 0, 'f', 0));

        // Connections count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + 20, barW, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString("%1 conn").arg(e.connections));

        show++;
    }
}

void PaperLoadBalancer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = {"Web", "API", "Database", "Cache", "Queue"};
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
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 70, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperLoadBalancer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total",       QString::number(entries_.size()),                  QColor(59, 130, 246)},
        {"Healthy",     QString("%1 / %2").arg(healthyCount())
                                                     .arg(entries_.size()), QColor(22, 163, 74)},
        {"Avg Load",    QString::number(avgLoad() * 100, 'f', 1) + "%",   QColor(217, 119, 6)},
        {"Categories",  QString::number(categoryCounts().size()),          QColor(124, 58, 237)}
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

void PaperLoadBalancer::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Load Balancer Monitor");
        return;
    }
    infoLabel_->setText(QString("%1 servers | %2 healthy | %3% avg load")
        .arg(entries_.size())
        .arg(healthyCount())
        .arg(avgLoad() * 100, 0, 'f', 1));
}

void PaperLoadBalancer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LoadBalancerEntry e;
        e.id          = settings_.value("id").toInt();
        e.server      = settings_.value("server").toString();
        e.category    = settings_.value("category").toString();
        e.algorithm   = settings_.value("algorithm").toString();
        e.load        = settings_.value("load").toReal();
        e.connections = settings_.value("connections").toInt();
        e.healthy     = settings_.value("healthy").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLoadBalancer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",          entries_[i].id);
        settings_.setValue("server",      entries_[i].server);
        settings_.setValue("category",    entries_[i].category);
        settings_.setValue("algorithm",   entries_[i].algorithm);
        settings_.setValue("load",        entries_[i].load);
        settings_.setValue("connections", entries_[i].connections);
        settings_.setValue("healthy",     entries_[i].healthy);
        settings_.setValue("color",       entries_[i].color.name());
    }
    settings_.endArray();
}
