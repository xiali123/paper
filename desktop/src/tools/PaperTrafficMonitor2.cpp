#include "tools/PaperTrafficMonitor2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperTrafficMonitor2::PaperTrafficMonitor2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TrafficMonitor2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList categories = {"Web", "API", "Internal", "External", "Admin"};
        QStringList methods = {"GET", "POST", "PUT", "DELETE"};
        QStringList routes = {"/api/papers", "/api/search", "/api/citations", "/api/auth"};
        QColor colors[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
                           QColor("#dc2626"), QColor("#7c3aed")};
        for (int i = 0; i < 8; ++i) {
            TrafficMonitor2Entry e;
            e.id = i + 1;
            e.route = routes[QRandomGenerator::global()->bounded(routes.size())];
            e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            e.method = methods[QRandomGenerator::global()->bounded(methods.size())];
            e.rps = 10.0 + QRandomGenerator::global()->bounded(990);
            e.errors = QRandomGenerator::global()->bounded(50);
            e.overloaded = e.rps > 800.0;
            e.color = colors[QRandomGenerator::global()->bounded(5)];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperTrafficMonitor2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Web", "API", "Internal", "External", "Admin"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search routes...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    monitorBtn_ = new QPushButton("Monitor");
    monitorBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(monitorBtn_, &QPushButton::clicked, this, &PaperTrafficMonitor2::onMonitor);
    toolbar->addWidget(monitorBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTrafficMonitor2::onClear);
    toolbar->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Traffic Monitor 2");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    setMinimumSize(640, 520);
}

void PaperTrafficMonitor2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No traffic data - click Monitor");
        return;
    }

    int w = width();
    int h = height();

    int leftW = static_cast<int>(w * 0.6);
    int rightW = w - leftW;
    int bottomH = static_cast<int>(h * 0.25);

    drawMonitorView(p, QRect(0, 0, leftW, h - bottomH));
    drawCategoryChart(p, QRect(leftW, 0, rightW, h - bottomH));
    drawStats(p, QRect(0, h - bottomH, w, bottomH));
}

void PaperTrafficMonitor2::drawMonitorView(QPainter& p, const QRect& rect) {
    int margin = 12;
    int x = rect.x() + margin;
    int y = rect.y() + margin;
    int usableW = rect.width() - 2 * margin;

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(x, y, usableW, 24, Qt::AlignVCenter, "Traffic Monitor");
    y += 30;

    int show = qMin(entries_.size(), 12);
    int itemH = qMin(38, (rect.height() - 50 - margin) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];

        // Row background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(x, y, usableW, itemH, 4, 4);

        // Color accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(x, y, 4, itemH, 2, 2);

        // Method badge
        QColor methodColor;
        if (e.method == "GET")       methodColor = QColor("#16a34a");
        else if (e.method == "POST") methodColor = QColor("#3b82f6");
        else if (e.method == "PUT")  methodColor = QColor("#d97706");
        else                         methodColor = QColor("#dc2626");

        int badgeX = x + 12;
        int badgeW = 48;
        p.setBrush(methodColor);
        p.drawRoundedRect(badgeX, y + (itemH - 18) / 2, badgeW, 18, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(badgeX, y + (itemH - 18) / 2, badgeW, 18,
                   Qt::AlignCenter, e.method);

        // Route path
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        int routeX = badgeX + badgeW + 8;
        int routeW = usableW * 0.35;
        p.drawText(routeX, y, routeW, itemH, Qt::AlignVCenter, e.route);

        // RPS bar
        int barX = routeX + routeW + 8;
        qreal maxRps = 1000.0;
        int barMaxW = usableW * 0.25;
        int barW = static_cast<int>((qBound(0.0, e.rps / maxRps, 1.0)) * barMaxW);
        int barH = 10;
        int barY = y + (itemH - barH) / 2;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barMaxW, barH, 3, 3);
        p.setBrush(e.overloaded ? QColor("#dc2626") : e.color);
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);

        // RPS value
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(barX + barMaxW + 4, y, 55, itemH,
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(e.rps, 'f', 1) + " rps");

        // Error count
        int errX = barX + barMaxW + 62;
        p.setPen(e.errors > 25 ? QColor("#dc2626") : QColor(100, 116, 139));
        p.drawText(errX, y, 50, itemH, Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(e.errors) + " err");

        // Overloaded warning
        if (e.overloaded) {
            int warnX = errX + 55;
            p.setPen(QColor("#dc2626"));
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(warnX, y, 60, itemH, Qt::AlignVCenter, "OVERLOAD");
        }

        y += itemH + 3;
    }
}

void PaperTrafficMonitor2::drawCategoryChart(QPainter& p, const QRect& rect) {
    int margin = 12;
    int x = rect.x() + margin;
    int y = rect.y() + margin;
    int usableW = rect.width() - 2 * margin;

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(x, y, usableW, 22, Qt::AlignVCenter, "Method Distribution");
    y += 28;

    // Count methods
    QMap<QString, int> methodMap;
    for (const auto& e : entries_) methodMap[e.method]++;
    int total = entries_.size();

    // Pie chart
    int chartSize = qMin(usableW, rect.height() - 100);
    int cx = x + usableW / 2;
    int cy = y + chartSize / 2;
    int radius = chartSize / 2 - 4;

    QMap<QString, QColor> methodColors;
    methodColors["GET"] = QColor("#16a34a");
    methodColors["POST"] = QColor("#3b82f6");
    methodColors["PUT"] = QColor("#d97706");
    methodColors["DELETE"] = QColor("#dc2626");

    qreal startAngle = 0.0;
    for (auto it = methodMap.constBegin(); it != methodMap.constEnd(); ++it) {
        qreal span = (static_cast<qreal>(it.value()) / total) * 360.0;
        p.setPen(Qt::NoPen);
        p.setBrush(methodColors.value(it.key(), QColor("#7c3aed")));
        p.drawPie(cx - radius, cy - radius, radius * 2, radius * 2,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    // Inner circle for donut effect
    int innerR = radius * 0.5;
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center label
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(cx - innerR, cy - 10, innerR * 2, 20, Qt::AlignCenter,
               QString::number(total));

    // Legend below chart
    int legendY = cy + radius + 12;
    p.setFont(QFont("Arial", 8));
    int colW = usableW / 2;
    int idx = 0;
    for (auto it = methodMap.constBegin(); it != methodMap.constEnd(); ++it) {
        int lx = x + (idx % 2) * colW;
        int ly = legendY + (idx / 2) * 18;
        p.setPen(Qt::NoPen);
        p.setBrush(methodColors.value(it.key(), QColor("#7c3aed")));
        p.drawRoundedRect(lx, ly + 2, 10, 10, 2, 2);
        p.setPen(QColor(71, 85, 105));
        p.drawText(lx + 14, ly, colW - 14, 16, Qt::AlignVCenter,
                   it.key() + " (" + QString::number(it.value()) + ")");
        ++idx;
    }
}

void PaperTrafficMonitor2::drawStats(QPainter& p, const QRect& rect) {
    int margin = 12;
    int x = rect.x() + margin;
    int y = rect.y() + margin;
    int usableW = rect.width() - 2 * margin;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Routes", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Overloaded",   QString::number(overloadedCount()), QColor("#dc2626")},
        {"Avg RPS",      QString::number(avgRps(), 'f', 1), QColor("#d97706")},
        {"Total Errors", QString::number(std::accumulate(
                             entries_.cbegin(), entries_.cend(), 0,
                             [](int sum, const TrafficMonitor2Entry& e) {
                                 return sum + e.errors;
                             })), QColor("#7c3aed")}
    };

    int boxCount = stats.size();
    int gap = 8;
    int boxW = (usableW - gap * (boxCount - 1)) / boxCount;
    int boxH = rect.height() - 2 * margin;

    for (int i = 0; i < boxCount; ++i) {
        int bx = x + i * (boxW + gap);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, y, boxW, boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(bx + 10, y + 6, boxW - 20, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 10, y + boxH / 2, boxW - 20, boxH / 2 - 6,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTrafficMonitor2::onMonitor() {
    QString text = inputField_->text().trimmed();

    QStringList categories = {"Web", "API", "Internal", "External", "Admin"};
    QStringList methods = {"GET", "POST", "PUT", "DELETE"};
    QStringList routes = {"/api/papers", "/api/search", "/api/citations", "/api/auth"};
    QColor colors[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
                       QColor("#dc2626"), QColor("#7c3aed")};

    int cIdx = categoryCombo_->currentIndex();

    TrafficMonitor2Entry e;
    e.id = entries_.size() + 1;
    e.route = text.isEmpty() ? routes[QRandomGenerator::global()->bounded(routes.size())] : text;
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                           : categories[cIdx - 1];
    e.method = methods[QRandomGenerator::global()->bounded(methods.size())];
    e.rps = 10.0 + QRandomGenerator::global()->bounded(990);
    e.errors = QRandomGenerator::global()->bounded(50);
    e.overloaded = e.rps > 800.0;
    e.color = colors[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
    inputField_->clear();
}

void PaperTrafficMonitor2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperTrafficMonitor2::addEntry(const TrafficMonitor2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    if (entry.overloaded) {
        emit spikeDetected(entry.id, entry.rps);
    }
    update();
}

QList<TrafficMonitor2Entry> PaperTrafficMonitor2::entries() const {
    return entries_;
}

int PaperTrafficMonitor2::overloadedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.overloaded) c++;
    return c;
}

qreal PaperTrafficMonitor2::avgRps() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) total += e.rps;
    return total / entries_.size();
}

QMap<QString, int> PaperTrafficMonitor2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTrafficMonitor2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Traffic Monitor 2");
        return;
    }
    infoLabel_->setText(
        QString("%1 routes | %2 overloaded | avg %3 rps | %4 errors")
            .arg(entries_.size())
            .arg(overloadedCount())
            .arg(avgRps(), 0, 'f', 1)
            .arg(std::accumulate(entries_.cbegin(), entries_.cend(), 0,
                                 [](int s, const TrafficMonitor2Entry& e) {
                                     return s + e.errors;
                                 })));
}

void PaperTrafficMonitor2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TrafficMonitor2Entry e;
        e.id = settings_.value("id").toInt();
        e.route = settings_.value("route").toString();
        e.category = settings_.value("category").toString();
        e.method = settings_.value("method").toString();
        e.rps = settings_.value("rps").toReal();
        e.errors = settings_.value("errors").toInt();
        e.overloaded = settings_.value("overloaded").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTrafficMonitor2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("route", entries_[i].route);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("method", entries_[i].method);
        settings_.setValue("rps", entries_[i].rps);
        settings_.setValue("errors", entries_[i].errors);
        settings_.setValue("overloaded", entries_[i].overloaded);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
