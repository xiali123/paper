#include "tools/PaperPortScanner.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperPortScanner::PaperPortScanner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PortScanner")
{
    setupUI();
    loadSettings();
}

void PaperPortScanner::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Web", "Mail", "Database", "Remote", "DNS"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Host address...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    scanBtn_ = new QPushButton("Scan");
    scanBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scanBtn_, &QPushButton::clicked, this, &PaperPortScanner::onScan);
    leftPanel->addWidget(scanBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; padding: 4px 12px; border-radius: 4px; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPortScanner::onClear);
    leftPanel->addWidget(clearBtn_);

    leftPanel->addStretch();

    infoLabel_ = new QLabel("Port Scanner");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    mainLayout->addLayout(leftPanel, 1);

    // Right area reserved for custom painting
    mainLayout->addStretch(3);

    setMinimumSize(700, 480);
}

void PaperPortScanner::addEntry(const PortEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<PortEntry> PaperPortScanner::entries() const {
    return entries_;
}

int PaperPortScanner::openCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.open) ++c;
    return c;
}

qreal PaperPortScanner::avgLatency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.latency;
    return sum / entries_.size();
}

QMap<QString, int> PaperPortScanner::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPortScanner::onScan() {
    QString host = inputField_->text().trimmed();
    if (host.isEmpty()) return;

    static const QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    QStringList services = {"HTTP", "SSH", "FTP", "SMTP", "DNS", "MySQL"};
    QStringList categories = {"Web", "Mail", "Database", "Remote", "DNS"};

    PortEntry entry;
    entry.id       = entries_.size() + 1;
    entry.host     = host;
    entry.category = categoryCombo_->currentText();
    entry.port     = 1 + QRandomGenerator::global()->bounded(65535);
    entry.latency  = 1.0 + QRandomGenerator::global()->bounded(500);
    entry.open     = QRandomGenerator::global()->bounded(2) == 0;
    entry.service  = services[QRandomGenerator::global()->bounded(services.size())];
    entry.color    = palette[QRandomGenerator::global()->bounded(5)];

    addEntry(entry);
    emit portScanned(entry.id, entry.latency);
    inputField_->clear();
}

void PaperPortScanner::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
    repaint();
}

void PaperPortScanner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Port Scanner");
        return;
    }

    int w = width(), h = height();

    // Three-column layout
    int colW = (w - 80) / 3;
    drawPortList(p, QRect(20, 50, colW, h - 80));
    drawCategoryChart(p, QRect(30 + colW, 50, colW, h - 80));
    drawStats(p, QRect(40 + 2 * colW, 50, colW, h - 80));
}

void PaperPortScanner::drawPortList(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter,
               "Port Scanner");

    if (entries_.isEmpty()) return;

    int show = qMin(15, entries_.size());
    int itemH = qMin(30, (rect.height() - 40) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 30 + i * (itemH + 3);

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(e.open ? QColor(16, 163, 74, 30) : QColor(220, 38, 38, 30));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left color indicator
        p.setPen(Qt::NoPen);
        p.setBrush(e.open ? QColor(16, 163, 74) : QColor(220, 38, 38));
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Port number
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, 50, 14, Qt::AlignVCenter,
                   QString(":%1").arg(e.port));

        // Service badge
        QColor badgeColor;
        if (e.service == "HTTP") badgeColor = QColor(59, 130, 246);
        else if (e.service == "SSH") badgeColor = QColor(124, 58, 237);
        else if (e.service == "FTP") badgeColor = QColor(217, 119, 6);
        else if (e.service == "SMTP") badgeColor = QColor(22, 163, 74);
        else if (e.service == "DNS") badgeColor = QColor(100, 116, 139);
        else badgeColor = QColor(220, 38, 38);

        int badgeX = rect.x() + 62;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeColor);
        p.drawRoundedRect(badgeX, y + 3, 38, 13, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, y + 3, 38, 13, Qt::AlignCenter, e.service);

        // Latency bar
        int barMaxW = rect.width() / 3;
        int barW = static_cast<int>((e.latency / 500.0) * barMaxW);
        barW = qBound(2, barW, barMaxW);
        p.setPen(Qt::NoPen);
        p.setBrush(e.latency < 100 ? QColor(16, 163, 74) : (e.latency < 300 ? QColor(217, 119, 6) : QColor(220, 38, 38)));
        p.drawRoundedRect(rect.x() + 10, y + 18, barW, 5, 2, 2);

        // Latency text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 14 + barW, y + 19, rect.width() - 14 - barW - 10, 10,
                   Qt::AlignVCenter, QString::number(static_cast<int>(e.latency)) + "ms");

        // Open/Closed status
        p.setPen(e.open ? QColor(16, 163, 74) : QColor(220, 38, 38));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 50, y + 2, 46, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.open ? "Open" : "Closed");
    }
}

void PaperPortScanner::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter,
               "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Web", "Mail", "Database", "Remote", "DNS"};
    QString labels[] = {"Web", "Mail", "Database", "Remote", "DNS"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int chartTop = rect.y() + 34;
    int barH     = qMin(24, (rect.height() - 50) / 5);
    int maxBarW  = rect.width() - 100;

    for (int i = 0; i < 5; ++i) {
        int y     = chartTop + i * (barH + 6);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW  = static_cast<int>((static_cast<qreal>(count) / maxVal) * maxBarW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y + 2, barW, barH - 4, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + 64 + barW, y + 2, 30, barH - 4, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperPortScanner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Ports",  QString::number(entries_.size()),  QColor(59, 130, 246)},
        {"Open",         QString::number(openCount()),      QColor(22, 163, 74)},
        {"Avg Latency",  QString::number(avgLatency(), 'f', 1) + " ms", QColor(217, 119, 6)}
    };

    int boxH = qMin(60, (rect.height() - 20) / 3);

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(rect.x() + 12, y + 6, rect.width() - 24, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + boxH / 2, rect.width() - 24, boxH / 2 - 4,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperPortScanner::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Port Scanner");
        return;
    }
    infoLabel_->setText(QString("Ports: %1 | Open: %2 | Avg: %3ms")
        .arg(entries_.size())
        .arg(openCount())
        .arg(avgLatency(), 0, 'f', 1));
}

void PaperPortScanner::loadSettings() {
    settings_.beginGroup("PortScanner");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PortEntry e;
        e.id       = settings_.value("id").toInt();
        e.host     = settings_.value("host").toString();
        e.category = settings_.value("category").toString();
        e.service  = settings_.value("service").toString();
        e.port     = settings_.value("port").toInt();
        e.latency  = settings_.value("latency").toDouble();
        e.open     = settings_.value("open").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperPortScanner::saveSettings() {
    settings_.beginGroup("PortScanner");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("host",     entries_[i].host);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("service",  entries_[i].service);
        settings_.setValue("port",     entries_[i].port);
        settings_.setValue("latency",  entries_[i].latency);
        settings_.setValue("open",     entries_[i].open);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
