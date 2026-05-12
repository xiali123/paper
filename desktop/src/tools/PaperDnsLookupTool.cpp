#include "tools/PaperDnsLookupTool.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperDnsLookupTool::PaperDnsLookupTool(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DnsLookupTool")
{
    setupUI();
    loadSettings();
}

void PaperDnsLookupTool::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"A", "AAAA", "MX", "CNAME", "TXT"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Domain name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    lookupBtn_ = new QPushButton("Lookup");
    lookupBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(lookupBtn_, &QPushButton::clicked, this, &PaperDnsLookupTool::onLookup);
    toolbar->addWidget(lookupBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDnsLookupTool::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("DNS Lookup");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch();

    setMinimumSize(580, 480);
}

void PaperDnsLookupTool::addEntry(const DnsEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<DnsEntry> PaperDnsLookupTool::entries() const { return entries_; }

int PaperDnsLookupTool::resolvedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.resolved) c++;
    return c;
}

qreal PaperDnsLookupTool::avgLatency() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.latency;
    return sum / entries_.size();
}

QMap<QString, int> PaperDnsLookupTool::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDnsLookupTool::onLookup() {
    QString domain = inputField_->text().trimmed();
    if (domain.isEmpty()) return;

    QStringList categories = {"A", "AAAA", "MX", "CNAME", "TXT"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        DnsEntry e;
        e.id = entries_.size() + 1;
        e.domain = domain;
        e.category = categories[categoryCombo_->currentIndex()];
        e.recordType = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.latency = 1 + QRandomGenerator::global()->bounded(200);
        e.ttl = 60 + QRandomGenerator::global()->bounded(86400 - 60 + 1);
        e.resolved = QRandomGenerator::global()->bounded(2) == 0;
        e.color = e.resolved ? colors[0] : colors[3];
        addEntry(e);
        emit lookupComplete(e.id, e.latency);
    }
    inputField_->clear();
}

void PaperDnsLookupTool::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperDnsLookupTool::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "DNS Lookup");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "DNS Lookup");

    int w = width(), h = height();
    drawDnsList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDnsLookupTool::drawDnsList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(e.resolved ? QColor(16,163,74,30) : QColor(220,38,38,30));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left color indicator
        p.setPen(Qt::NoPen);
        p.setBrush(e.resolved ? QColor(16,163,74) : QColor(220,38,38));
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Domain and record type badge
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.domain.left(20));

        // Record type badge
        QColor badgeColor;
        if (e.recordType == "A") badgeColor = QColor(59,130,246);
        else if (e.recordType == "AAAA") badgeColor = QColor(124,58,237);
        else if (e.recordType == "MX") badgeColor = QColor(217,119,6);
        else if (e.recordType == "CNAME") badgeColor = QColor(22,163,74);
        else badgeColor = QColor(100,116,139);

        int badgeX = rect.x() + rect.width() / 2 - 50;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeColor);
        p.drawRoundedRect(badgeX, y + 4, 36, 14, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, y + 4, 36, 14, Qt::AlignCenter, e.recordType);

        // Latency bar
        int barMaxW = rect.width() / 3;
        int barW = static_cast<int>((e.latency / 200.0) * barMaxW);
        barW = qBound(2, barW, barMaxW);
        p.setPen(Qt::NoPen);
        p.setBrush(e.latency < 50 ? QColor(16,163,74) : (e.latency < 100 ? QColor(217,119,6) : QColor(220,38,38)));
        p.drawRoundedRect(rect.x() + 10, y + 20, barW, 6, 3, 3);

        // Latency and TTL text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 14 + barW, y + 22, rect.width() - 14 - barW - 10, 12,
                   Qt::AlignVCenter, QString::number(static_cast<int>(e.latency)) + "ms | TTL " + QString::number(e.ttl) + "s");

        // Resolved/timeout status
        p.setPen(e.resolved ? QColor(16,163,74) : QColor(220,38,38));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 60, y + 4, 56, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.resolved ? "Resolved" : "Timeout");
    }
}

void PaperDnsLookupTool::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"A", "AAAA", "MX", "CNAME", "TXT"};
    QString labels[] = {"A", "AAAA", "MX", "CNAME", "TXT"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

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

void PaperDnsLookupTool::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Lookups", QString::number(entries_.size()), QColor(59,130,246)},
        {"Resolved", QString::number(resolvedCount()), QColor(16,163,74)},
        {"Avg Latency", QString::number(avgLatency(), 'f', 1) + "ms", QColor(217,119,6)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 3);
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

void PaperDnsLookupTool::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("DNS Lookup"); return; }
    infoLabel_->setText(QString("Lookups: %1 | Resolved: %2 | Avg: %3ms")
        .arg(entries_.size()).arg(resolvedCount()).arg(avgLatency(), 0, 'f', 1));
}

void PaperDnsLookupTool::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DnsEntry e;
        e.id = settings_.value("id").toInt();
        e.domain = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.recordType = settings_.value("recordType").toString();
        e.latency = settings_.value("latency").toDouble();
        e.ttl = settings_.value("ttl").toInt();
        e.resolved = settings_.value("resolved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDnsLookupTool::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("domain", entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("recordType", entries_[i].recordType);
        settings_.setValue("latency", entries_[i].latency);
        settings_.setValue("ttl", entries_[i].ttl);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
