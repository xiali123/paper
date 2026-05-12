#include "tools/PaperPortRadar.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperPortRadar::PaperPortRadar(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PortRadar")
{
    setupUI();
    loadSettings();
}

void PaperPortRadar::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Top toolbar row
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "HTTP", "SSH", "Database", "Custom"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 100px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter port...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    scanBtn_ = new QPushButton("Scan");
    scanBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(scanBtn_, &QPushButton::clicked, this, &PaperPortRadar::onScan);
    toolbar->addWidget(scanBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #dc2626; "
        "border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPortRadar::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Port Radar");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    // Painting area stretches to fill the rest
    mainLayout->addStretch(1);

    setMinimumSize(720, 520);
}

// ── data helpers ──────────────────────────────────────────────────────

void PaperPortRadar::addEntry(const PortRadarEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<PortRadarEntry> PaperPortRadar::entries() const {
    return entries_;
}

int PaperPortRadar::openCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.open) ++c;
    return c;
}

qreal PaperPortRadar::avgLatency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.latency;
    return sum / entries_.size();
}

QMap<QString, int> PaperPortRadar::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ── slots ─────────────────────────────────────────────────────────────

void PaperPortRadar::onScan() {
    QString portText = inputField_->text().trimmed();

    static const QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6 HTTP
        QColor(22, 163, 74),    // #16a34a SSH
        QColor(124, 58, 237),   // #7c3aed Database
        QColor(217, 119, 6)     // #d97706 Custom
    };

    QStringList services = {"HTTP", "SSH", "MySQL", "PostgreSQL", "Redis", "MongoDB"};
    QStringList categories = {"HTTP", "SSH", "Database", "Custom"};

    QString category = categoryCombo_->currentText();
    int catIdx = category == "HTTP" ? 0 : category == "SSH" ? 1
                 : category == "Database"                   ? 2
                                                            : 3;

    PortRadarEntry entry;
    entry.id         = entries_.size() + 1;
    entry.port       = portText.isEmpty()
                           ? QString::number(1 + QRandomGenerator::global()->bounded(65535))
                           : portText;
    entry.category   = category == "All" ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                         : category;
    entry.service    = services[QRandomGenerator::global()->bounded(services.size())];
    entry.latency    = 1.0 + QRandomGenerator::global()->bounded(500);
    entry.connections = QRandomGenerator::global()->bounded(200);
    entry.open       = QRandomGenerator::global()->bounded(2) == 0;
    entry.color      = palette[catIdx];

    addEntry(entry);
    emit portScanned(entry.id, entry.latency);
    inputField_->clear();
}

void PaperPortRadar::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperPortRadar::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Port Radar");
        return;
    }
    infoLabel_->setText(QString("Ports: %1 | Open: %2 | Avg latency: %3 ms")
        .arg(entries_.size())
        .arg(openCount())
        .arg(avgLatency(), 0, 'f', 1));
}

// ── persistence ───────────────────────────────────────────────────────

void PaperPortRadar::loadSettings() {
    settings_.beginGroup("PortRadar");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PortRadarEntry e;
        e.id          = settings_.value("id").toInt();
        e.port        = settings_.value("port").toString();
        e.category    = settings_.value("category").toString();
        e.service     = settings_.value("service").toString();
        e.latency     = settings_.value("latency").toDouble();
        e.connections = settings_.value("connections").toInt();
        e.open        = settings_.value("open").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperPortRadar::saveSettings() {
    settings_.beginGroup("PortRadar");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",          entries_[i].id);
        settings_.setValue("port",        entries_[i].port);
        settings_.setValue("category",    entries_[i].category);
        settings_.setValue("service",     entries_[i].service);
        settings_.setValue("latency",     entries_[i].latency);
        settings_.setValue("connections", entries_[i].connections);
        settings_.setValue("open",        entries_[i].open);
        settings_.setValue("color",       entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

// ── painting ──────────────────────────────────────────────────────────

void PaperPortRadar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 0, width() - 40, 28, Qt::AlignLeft | Qt::AlignVCenter, "Port Radar");

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No ports scanned yet");
        return;
    }

    int w = width();
    int h = height();

    // Layout: radar view on top half, chart bottom-left, stats bottom-right
    int toolbarH = 70;   // approximate height of toolbar + info label
    int topH     = (h - toolbarH) * 55 / 100;
    int bottomH  = (h - toolbarH) - topH - 10;

    drawRadarView(p, QRect(10, toolbarH, w - 20, topH));
    drawCategoryChart(p, QRect(10, toolbarH + topH + 5, (w - 30) / 2, bottomH));
    drawStats(p, QRect(20 + (w - 30) / 2, toolbarH + topH + 5, (w - 30) / 2, bottomH));
}

void PaperPortRadar::drawRadarView(QPainter& p, const QRect& area) {
    // Card background
    QPainterPath card;
    card.addRoundedRect(area, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawPath(card);

    // Card border
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(card);

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(area.x() + 12, area.y() + 4, area.width() - 24, 22,
               Qt::AlignLeft | Qt::AlignVCenter, "Scanned Ports");

    // Filter by current combo selection
    QString filter = categoryCombo_->currentText();
    QList<PortRadarEntry> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter)
            visible.append(e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(160, 174, 192));
        p.setFont(QFont("Arial", 9));
        p.drawText(area, Qt::AlignCenter, "No entries for this category");
        return;
    }

    int maxRows    = qMin(10, visible.size());
    int headerH    = 30;
    int rowH       = qMin(28, (area.height() - headerH - 12) / qMax(maxRows, 1));
    int startX     = area.x() + 12;
    int rowY       = area.y() + headerH + 4;

    // Category → colour map
    auto catColor = [](const QString& cat) -> QColor {
        if (cat == "HTTP")     return QColor(59, 130, 246);   // #3b82f6
        if (cat == "SSH")      return QColor(22, 163, 74);    // #16a34a
        if (cat == "Database") return QColor(124, 58, 237);   // #7c3aed
        return QColor(217, 119, 6);                           // #d97706 Custom
    };

    for (int i = 0; i < maxRows; ++i) {
        const auto& e = visible[i];
        int y = rowY + i * (rowH + 3);

        // Row background
        p.setPen(Qt::NoPen);
        p.setBrush(e.open ? QColor(16, 163, 74, 20) : QColor(220, 38, 38, 20));
        QPainterPath rowBg;
        rowBg.addRoundedRect(startX, y, area.width() - 24, rowH, 4, 4);
        p.drawPath(rowBg);

        // Open/closed indicator dot
        p.setBrush(e.open ? QColor(16, 163, 74) : QColor(220, 38, 38));
        p.drawEllipse(startX + 6, y + rowH / 2 - 4, 8, 8);

        // Port number
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(startX + 20, y + 2, 52, rowH - 4, Qt::AlignVCenter,
                   QString(":%1").arg(e.port));

        // Service name badge
        QColor badgeCol = catColor(e.category);
        int badgeX = startX + 76;
        int badgeW = 58;
        int badgeH = qMin(16, rowH - 6);
        int badgeY = y + (rowH - badgeH) / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeCol);
        QPainterPath badge;
        badge.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 3, 3);
        p.drawPath(badge);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter, e.service);

        // Latency bar
        int barAreaX = startX + 140;
        int barMaxW  = (area.width() - 24 - 140 - 120);
        int barW     = static_cast<int>((e.latency / 500.0) * barMaxW);
        barW = qBound(4, barW, barMaxW);
        int barY = y + (rowH / 2) - 3;

        p.setPen(Qt::NoPen);
        QColor barCol = e.latency < 100 ? QColor(16, 163, 74)
                      : e.latency < 300 ? QColor(217, 119, 6)
                                        : QColor(220, 38, 38);
        p.setBrush(barCol);
        QPainterPath barPath;
        barPath.addRoundedRect(barAreaX, barY, barW, 6, 3, 3);
        p.drawPath(barPath);

        // Latency ms label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barAreaX + barW + 4, barY - 1, 44, 10, Qt::AlignVCenter,
                   QString::number(static_cast<int>(e.latency)) + " ms");

        // Connections count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + area.width() - 80, y + 2, 56, rowH - 4,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.connections) + " conn");
    }
}

void PaperPortRadar::drawCategoryChart(QPainter& p, const QRect& area) {
    // Card background
    QPainterPath card;
    card.addRoundedRect(area, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawPath(card);
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(card);

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(area.x() + 12, area.y() + 4, area.width() - 24, 22,
               Qt::AlignLeft | Qt::AlignVCenter, "Category Breakdown");

    auto counts = categoryCounts();
    struct CatInfo { QString label; QColor color; };
    CatInfo cats[] = {
        {"HTTP",     QColor(59, 130, 246)},
        {"SSH",      QColor(22, 163, 74)},
        {"Database", QColor(124, 58, 237)},
        {"Custom",   QColor(217, 119, 6)}
    };

    int maxVal = 1;
    for (const auto& c : cats)
        maxVal = qMax(maxVal, counts.contains(c.label) ? counts[c.label] : 0);

    int chartTop = area.y() + 32;
    int barH     = qMin(22, (area.height() - 50) / 4);
    int maxBarW  = area.width() - 120;
    int labelW   = 56;

    for (int i = 0; i < 4; ++i) {
        int y     = chartTop + i * (barH + 8);
        int count = counts.contains(cats[i].label) ? counts[cats[i].label] : 0;
        int barW  = static_cast<int>((static_cast<qreal>(count) / maxVal) * maxBarW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + 12, y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter,
                   cats[i].label);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(cats[i].color);
        QPainterPath bar;
        bar.addRoundedRect(area.x() + 12 + labelW + 8, y + 2, barW, barH - 4, 3, 3);
        p.drawPath(bar);

        // Count badge
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(area.x() + 12 + labelW + 12 + barW, y + 2, 30, barH - 4,
                   Qt::AlignVCenter, QString::number(count));
    }
}

void PaperPortRadar::drawStats(QPainter& p, const QRect& area) {
    // Card background
    QPainterPath card;
    card.addRoundedRect(area, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawPath(card);
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawPath(card);

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(area.x() + 12, area.y() + 4, area.width() - 24, 22,
               Qt::AlignLeft | Qt::AlignVCenter, "Summary");

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Ports",  QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Open Ports",   QString::number(openCount()),     QColor(22, 163, 74)},
        {"Avg Latency",  QString::number(avgLatency(), 'f', 1) + " ms", QColor(217, 119, 6)}
    };

    int boxH = qMin(50, (area.height() - 50) / 3);
    int innerX = area.x() + 12;
    int innerW = area.width() - 24;

    for (int i = 0; i < stats.size(); ++i) {
        int y = area.y() + 32 + i * (boxH + 8);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath pill;
        pill.addRoundedRect(innerX, y, innerW, boxH, 6, 6);
        p.drawPath(pill);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(innerX + 10, y + 4, innerW - 20, boxH / 2,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(innerX + 10, y + boxH / 2, innerW - 20, boxH / 2 - 4,
                   Qt::AlignVCenter, stats[i].label);
    }
}
