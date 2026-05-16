#include "tools/PaperTraceroute.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTraceroute::PaperTraceroute(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "Traceroute")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        struct Seed { QString destination; QString category; };
        Seed seeds[] = {
            {"api.internal.corp",       "Internal"},
            {"db.replica.internal",     "Internal"},
            {"gateway.external.net",    "External"},
            {"cdn.assets.global",       "CDN"},
            {"vpn.tunnel.secure",       "VPN"},
            {"cloud.compute.aws",       "Cloud"},
            {"storage.blob.azure",      "Cloud"},
            {"cdn.static.cdnfast",      "CDN"},
        };

        QColor catColors[] = {
            QColor(59, 130, 246),   // Internal  #3b82f6
            QColor(22, 163, 74),    // External  #16a34a
            QColor(217, 119, 6),    // VPN       #d97706
            QColor(220, 38, 38),    // Cloud     #dc2626
            QColor(124, 58, 237),   // CDN       #7c3aed
        };
        QMap<QString, int> colorMap = {
            {"Internal", 0}, {"External", 1}, {"VPN", 2},
            {"Cloud", 3}, {"CDN", 4}
        };

        auto* rng = QRandomGenerator::global();
        for (int i = 0; i < 8; ++i) {
            TracerouteEntry e;
            e.id = i + 1;
            e.destination = seeds[i].destination;
            e.category = seeds[i].category;
            e.path = seeds[i].destination;
            e.latency = rng->bounded(5, 300);
            e.hops = rng->bounded(1, 15);
            e.complete = rng->bounded(100) < 85;
            e.color = catColors[colorMap.value(e.category, 0)];
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperTraceroute::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Category:"));

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Internal", "External", "VPN", "Cloud", "CDN"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { update(); });
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter destination to trace...");
    toolbar->addWidget(inputField_, 2);

    traceBtn_ = new QPushButton("Trace");
    traceBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; border-radius: 4px; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(traceBtn_, &QPushButton::clicked, this, &PaperTraceroute::onTrace);
    toolbar->addWidget(traceBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTraceroute::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Traceroute ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 480);
}

void PaperTraceroute::addEntry(const TracerouteEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit routeTraced(entry.id, entry.latency);
    update();
}

QList<TracerouteEntry> PaperTraceroute::entries() const {
    return entries_;
}

int PaperTraceroute::completeCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.complete) c++;
    return c;
}

qreal PaperTraceroute::avgLatency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_)
        sum += e.latency;
    return sum / entries_.size();
}

QMap<QString, int> PaperTraceroute::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperTraceroute::onTrace() {
    QString dest = inputField_->text().trimmed();
    if (dest.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    if (category == "All") category = "External";

    QColor catColors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };
    QMap<QString, int> colorMap = {
        {"Internal", 0}, {"External", 1}, {"VPN", 2},
        {"Cloud", 3}, {"CDN", 4}
    };

    auto* rng = QRandomGenerator::global();

    TracerouteEntry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.destination = dest;
    e.category = category;
    e.path = dest;
    e.latency = rng->bounded(5, 500);
    e.hops = rng->bounded(1, 20);
    e.complete = rng->bounded(100) < 80;
    e.color = catColors[colorMap.value(category, 0)];

    addEntry(e);
    inputField_->clear();
}

void PaperTraceroute::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Traceroute ready");
    update();
}

void PaperTraceroute::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No trace entries -- click Trace to start");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Traceroute");

    int w = width(), h = height();
    int halfW = w / 2 - 25;
    drawTraceView(p, QRect(20, 50, halfW, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, halfW, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, halfW, h / 2 - 50));
}

void PaperTraceroute::drawTraceView(QPainter& p, const QRect& rect) {
    QString filter = categoryCombo_->currentText();

    QList<const TracerouteEntry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter)
            visible.append(&e);
    }

    int maxShow = qMin(8, visible.size());
    int itemH = qMin(52, (rect.height() - 10) / qMax(maxShow, 1));
    if (itemH < 24) itemH = 24;

    for (int i = 0; i < maxShow; ++i) {
        const auto* e = visible[visible.size() - 1 - i];
        int y = rect.y() + i * (itemH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e->complete ? QColor(248, 250, 252) : QColor(254, 242, 242));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Color indicator bar on left
        p.setBrush(e->color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Destination name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 14, y + 2, rect.width() * 0.45, 16,
                   Qt::AlignVCenter, e->destination.left(30));

        // Category + hops info
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 14, y + 18, rect.width() * 0.45, 14,
                   Qt::AlignVCenter,
                   e->category + " | Hops: " + QString::number(e->hops)
                   + " | " + QString::number(static_cast<int>(e->latency)) + "ms");

        // Hop-by-hop node chain
        int chainX = rect.x() + rect.width() * 0.52;
        int chainW = rect.width() * 0.44;
        int nodeCount = qMin(e->hops, 10);
        int nodeSpacing = nodeCount > 1 ? chainW / (nodeCount - 1) : 0;
        int nodeR = qMax(3, qMin(6, nodeSpacing / 3));
        int nodeY = y + itemH / 2;

        // Draw connecting lines first
        if (nodeCount > 1) {
            QPen linePen(e->color.lighter(150), 2);
            p.setPen(linePen);
            p.drawLine(chainX, nodeY, chainX + (nodeCount - 1) * nodeSpacing, nodeY);
        }

        // Draw hop nodes
        for (int h = 0; h < nodeCount; ++h) {
            int nx = chainX + h * nodeSpacing;

            // Node circle
            p.setPen(Qt::NoPen);
            if (h == 0) {
                // Source node
                p.setBrush(e->complete ? QColor(34, 197, 94) : QColor(239, 68, 68));
            } else if (h == nodeCount - 1) {
                // Destination node
                p.setBrush(e->complete ? e->color : QColor(239, 68, 68));
            } else {
                // Intermediate hop
                p.setBrush(e->color.lighter(130));
            }
            p.drawEllipse(nx - nodeR, nodeY - nodeR, nodeR * 2, nodeR * 2);
        }

        // Complete / failed indicator
        if (!e->complete) {
            QPen failPen(QColor(220, 38, 38), 1);
            p.setPen(failPen);
            int lastX = chainX + (nodeCount - 1) * nodeSpacing;
            p.drawLine(lastX - 3, nodeY - 3, lastX + 3, nodeY + 3);
            p.drawLine(lastX + 3, nodeY - 3, lastX - 3, nodeY + 3);
        }
    }
}

void PaperTraceroute::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList categories = {"Internal", "External", "VPN", "Cloud", "CDN"};
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
               Qt::AlignHCenter | Qt::AlignTop, "routes");
}

void PaperTraceroute::drawStats(QPainter& p, const QRect& rect) {
    int totalHops = 0;
    for (const auto& e : entries_)
        totalHops += e.hops;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total",        QString::number(entries_.size()),   QColor(59, 130, 246)},
        {"Complete",     QString::number(completeCount()),   QColor(22, 163, 74)},
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

void PaperTraceroute::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Traceroute ready");
        return;
    }
    infoLabel_->setText(QString("%1 routes | %2 complete | Avg %3ms")
        .arg(entries_.size())
        .arg(completeCount())
        .arg(static_cast<int>(avgLatency()));
}

void PaperTraceroute::loadSettings() {
    int size = settings_.beginReadArray("traceEntries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TracerouteEntry e;
        e.id = settings_.value("id").toInt();
        e.destination = settings_.value("destination").toString();
        e.category = settings_.value("category").toString();
        e.path = settings_.value("path").toString();
        e.latency = settings_.value("latency").toReal();
        e.hops = settings_.value("hops").toInt();
        e.complete = settings_.value("complete").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
}

void PaperTraceroute::saveSettings() {
    settings_.beginWriteArray("traceEntries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("destination", entries_[i].destination);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("path", entries_[i].path);
        settings_.setValue("latency", entries_[i].latency);
        settings_.setValue("hops", entries_[i].hops);
        settings_.setValue("complete", entries_[i].complete);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
