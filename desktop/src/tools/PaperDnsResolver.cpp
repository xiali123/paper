#include "tools/PaperDnsResolver.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperDnsResolver::PaperDnsResolver(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DnsResolver")
{
    setupUI();
    loadSettings();
}

void PaperDnsResolver::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"A Record", "AAAA Record", "CNAME", "MX", "TXT"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter domain to resolve...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    resolveBtn_ = new QPushButton("Resolve");
    resolveBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(resolveBtn_, &QPushButton::clicked, this, &PaperDnsResolver::onResolve);
    toolbar->addWidget(resolveBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDnsResolver::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("DNS Resolver");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch();

    setMinimumSize(620, 500);
}

void PaperDnsResolver::addEntry(const DnsResolverEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<DnsResolverEntry> PaperDnsResolver::entries() const { return entries_; }

int PaperDnsResolver::resolvedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.resolved) c++;
    return c;
}

qreal PaperDnsResolver::avgTtl() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.ttl;
    return sum / entries_.size();
}

QMap<QString, int> PaperDnsResolver::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDnsResolver::onResolve() {
    QString domain = inputField_->text().trimmed();
    if (domain.isEmpty()) return;

    QStringList categories = {"A Record", "AAAA Record", "CNAME", "MX", "TXT"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        DnsResolverEntry e;
        e.id = entries_.size() + 1;
        e.domain = domain;
        e.category = categories[categoryCombo_->currentIndex()];
        e.record = QString("%1.%2.%3.%4")
            .arg(QRandomGenerator::global()->bounded(256))
            .arg(QRandomGenerator::global()->bounded(256))
            .arg(QRandomGenerator::global()->bounded(256))
            .arg(QRandomGenerator::global()->bounded(256));
        e.ttl = 60 + QRandomGenerator::global()->bounded(86400 - 60 + 1);
        e.queries = 1 + QRandomGenerator::global()->bounded(50);
        e.resolved = QRandomGenerator::global()->bounded(5) != 0;
        e.color = colors[categoryCombo_->currentIndex()];
        addEntry(e);
        emit domainResolved(e.id, e.ttl);
    }
    inputField_->clear();
}

void PaperDnsResolver::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperDnsResolver::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "DNS Resolver");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "DNS Resolver");

    int w = width(), h = height();
    drawResolverView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDnsResolver::drawResolverView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(38, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(e.resolved ? QColor(16, 163, 74, 25) : QColor(220, 38, 38, 25));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left color indicator
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Domain name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter, e.domain.left(22));

        // Category badge
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        int badgeX = rect.x() + rect.width() / 2 - 60;
        p.drawRoundedRect(badgeX, y + 3, 50, 14, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, y + 3, 50, 14, Qt::AlignCenter, e.category);

        // TTL bar
        int barMaxW = rect.width() / 3;
        int barW = static_cast<int>((e.ttl / 86400.0) * barMaxW);
        barW = qBound(4, barW, barMaxW);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(130));
        p.drawRoundedRect(rect.x() + 10, y + 22, barW, 6, 3, 3);

        // Record and TTL text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 14 + barW, y + 24, rect.width() - 14 - barW - 10, 12,
                   Qt::AlignVCenter,
                   e.record + " | TTL " + QString::number(static_cast<int>(e.ttl)) + "s");

        // Queries and resolved status
        p.setPen(e.resolved ? QColor(16, 163, 74) : QColor(220, 38, 38));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 80, y + 2, 76, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.resolved ? "Resolved" : "Failed");

        // Query count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 80, y + 20, 76, 12,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "Q:" + QString::number(e.queries));
    }
}

void PaperDnsResolver::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"A Record", "AAAA Record", "CNAME", "MX", "TXT"};
    QColor colors[] = {
        QColor(59, 130, 246),
        QColor(22, 163, 74),
        QColor(217, 119, 6),
        QColor(220, 38, 38),
        QColor(124, 58, 237)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperDnsResolver::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries",   QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Resolved",        QString::number(resolvedCount()), QColor(22, 163, 74)},
        {"Avg TTL",         QString::number(avgTtl(), 'f', 0) + "s", QColor(217, 119, 6)}
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

void PaperDnsResolver::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("DNS Resolver");
        return;
    }
    infoLabel_->setText(QString("Entries: %1 | Resolved: %2 | Avg TTL: %3s")
        .arg(entries_.size())
        .arg(resolvedCount())
        .arg(avgTtl(), 0, 'f', 0));
}

void PaperDnsResolver::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DnsResolverEntry e;
        e.id       = settings_.value("id").toInt();
        e.domain   = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.record   = settings_.value("record").toString();
        e.ttl      = settings_.value("ttl").toDouble();
        e.queries  = settings_.value("queries").toInt();
        e.resolved = settings_.value("resolved").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDnsResolver::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("domain",   entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("record",   entries_[i].record);
        settings_.setValue("ttl",      entries_[i].ttl);
        settings_.setValue("queries",  entries_[i].queries);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
