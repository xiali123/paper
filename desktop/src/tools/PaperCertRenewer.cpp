#include "tools/PaperCertRenewer.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <numeric>

PaperCertRenewer::PaperCertRenewer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CertRenewer")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        const QStringList domains = {
            "api.example.com", "auth.service.io", "cdn.research.org",
            "db.internal.net", "mail.university.edu", "git.devops.local",
            "vpn.enterprise.io", "registry.docker.local"
        };
        const QStringList issuers = {
            "Let's Encrypt", "DigiCert", "Cloudflare"
        };
        const QStringList categories = {
            "Web", "API", "Database", "Email", "Internal"
        };
        QColor palette[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237)
        };

        for (int i = 0; i < 8; ++i) {
            CertRenewerEntry e;
            e.id = i + 1;
            e.domain = domains.at(i % domains.size());
            e.category = categories.at(i % categories.size());
            e.issuer = issuers.at(i % issuers.size());
            e.daysLeft = QRandomGenerator::global()->bounded(1, 90);
            e.renewals = QRandomGenerator::global()->bounded(0, 12);
            e.expiring = e.daysLeft <= 30;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperCertRenewer::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Web", "API", "Database", "Email", "Internal"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search domains...");
    toolbar->addWidget(inputField_, 1);

    renewBtn_ = new QPushButton("Renew");
    renewBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renewBtn_, &QPushButton::clicked, this, &PaperCertRenewer::onRenew);
    toolbar->addWidget(renewBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCertRenewer::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel();
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch(1);

    setMinimumSize(640, 480);
}

void PaperCertRenewer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No certificates - click Renew to add entries");
        return;
    }

    int w = width(), h = height();
    int topH = static_cast<int>(h * 0.75);
    int bottomH = h - topH;
    int leftW = static_cast<int>(w * 0.6);
    int rightW = w - leftW;

    drawRenewerView(p, QRect(10, 10, leftW - 15, topH - 20));
    drawCategoryChart(p, QRect(leftW + 5, 10, rightW - 15, topH - 20));
    drawStats(p, QRect(10, topH + 5, w - 20, bottomH - 15));
}

void PaperCertRenewer::drawRenewerView(QPainter& p, const QRect& r) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(r.x(), r.y() + 18, "Certificate Renewer");

    int startY = r.y() + 34;
    int maxShow = qMin(static_cast<int>(entries_.size()), 10);
    int itemH = qMin(48, (r.height() - startY + r.y() - 10) / qMax(maxShow, 1));
    int barMaxW = r.width() - 20;

    int shown = 0;
    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        int y = startY + shown * (itemH + 3);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.expiring ? QColor(254, 242, 242) : QColor(248, 250, 252));
        p.drawRoundedRect(r.x(), y, r.width(), itemH, 4, 4);

        // Days-left countdown bar
        QColor barColor;
        if (e.daysLeft > 30)      barColor = QColor(22, 163, 74);   // green
        else if (e.daysLeft > 7)  barColor = QColor(217, 119, 6);   // amber
        else                      barColor = QColor(220, 38, 38);    // red

        int barW = static_cast<int>((e.daysLeft / 90.0) * barMaxW);
        p.setBrush(barColor);
        p.setOpacity(0.6);
        p.drawRoundedRect(r.x() + 10, y + itemH - 8, qMax(barW, 4), 5, 2, 2);
        p.setOpacity(1.0);

        // Domain name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(r.x() + 10, y + 2, r.width() - 20, 16, Qt::AlignVCenter,
                   e.domain.left(30));

        // Issuer badge
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(r.x() + 10, y + 17, r.width() - 20, 14, Qt::AlignVCenter,
                   e.issuer.left(24) + " | Renewals: " + QString::number(e.renewals));

        // Expiring warning icon + days-left badge
        if (e.expiring) {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(r.x() + r.width() - 90, y + 3, 30, 14,
                       Qt::AlignVCenter | Qt::AlignRight, QString::fromUtf8("⚠"));
        }

        p.setPen(barColor);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(r.x() + r.width() - 55, y + 3, 50, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(static_cast<int>(e.daysLeft)) + "d");

        ++shown;
    }
}

void PaperCertRenewer::drawCategoryChart(QPainter& p, const QRect& r) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(r.topLeft(), "Certificate Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    QColor palette[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int cx = r.x() + r.width() / 2;
    int cy = r.y() + 30 + (r.height() - 60) / 2;
    int radius = qMin(r.width(), r.height() - 60) / 2 - 10;
    if (radius < 20) radius = 20;

    qreal startAngle = 0.0;
    int colorIdx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal span = (static_cast<qreal>(it.value()) / total) * 360.0;
        p.setPen(Qt::NoPen);
        p.setBrush(palette[colorIdx % 5]);
        p.drawPie(cx - radius, cy - radius, radius * 2, radius * 2,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
        ++colorIdx;
    }

    // Legend below the pie
    int legendY = cy + radius + 10;
    colorIdx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (legendY + 14 > r.y() + r.height()) break;
        int lx = r.x() + 10;
        p.setPen(Qt::NoPen);
        p.setBrush(palette[colorIdx % 5]);
        p.drawRoundedRect(lx, legendY, 10, 10, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(lx + 14, legendY + 10,
                   it.key() + " (" + QString::number(it.value()) + ")");
        legendY += 16;
        ++colorIdx;
    }
}

void PaperCertRenewer::drawStats(QPainter& p, const QRect& r) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Certs",    QString::number(entries_.size()),         QColor(59, 130, 246)},
        {"Expiring Soon",  QString::number(expiringCount()),         QColor(220, 38, 38)},
        {"Avg Days Left",  QString::number(avgDaysLeft(), 'f', 1),   QColor(217, 119, 6)},
        {"Total Renewals", QString::number(std::accumulate(
                               entries_.begin(), entries_.end(), 0,
                               [](int sum, const CertRenewerEntry& e) {
                                   return sum + e.renewals;
                               })),                                   QColor(124, 58, 237)},
    };

    int boxW = (r.width() - 30) / 4;
    int boxH = r.height() - 4;
    if (boxH > 60) boxH = 60;

    for (int i = 0; i < stats.size(); ++i) {
        int x = r.x() + i * (boxW + 10);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, r.y(), boxW, boxH, 6, 6);

        // Top color accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, r.y(), boxW, 3, 2, 2);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(x + 10, r.y() + 6, boxW - 20, 28, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, r.y() + 34, boxW - 20, 18, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperCertRenewer::onRenew() {
    const QStringList domains = {
        "api.example.com", "auth.service.io", "cdn.research.org",
        "db.internal.net", "mail.university.edu", "git.devops.local",
        "vpn.enterprise.io", "registry.docker.local",
        "staging.cloud.dev", "monitoring.ops.net"
    };
    const QStringList issuers = {"Let's Encrypt", "DigiCert", "Cloudflare"};
    const QStringList categories = {"Web", "API", "Database", "Email", "Internal"};
    QColor palette[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int idx = QRandomGenerator::global()->bounded(domains.size());
    CertRenewerEntry e;
    e.id = entries_.size() + 1;
    e.domain = domains.at(idx);
    e.category = categories.at(QRandomGenerator::global()->bounded(categories.size()));
    e.issuer = issuers.at(QRandomGenerator::global()->bounded(issuers.size()));
    e.daysLeft = QRandomGenerator::global()->bounded(1, 90);
    e.renewals = QRandomGenerator::global()->bounded(0, 12);
    e.expiring = e.daysLeft <= 30;
    e.color = palette[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
}

void PaperCertRenewer::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperCertRenewer::addEntry(const CertRenewerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit certRenewed(entry.id, entry.daysLeft);
    update();
}

QList<CertRenewerEntry> PaperCertRenewer::entries() const {
    return entries_;
}

int PaperCertRenewer::expiringCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.expiring) ++c;
    return c;
}

qreal PaperCertRenewer::avgDaysLeft() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.daysLeft;
    return sum / entries_.size();
}

QMap<QString, int> PaperCertRenewer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCertRenewer::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Certificate Renewer");
        return;
    }
    infoLabel_->setText(QString("Certs: %1 | Expiring: %2 | Avg Days: %3")
        .arg(entries_.size())
        .arg(expiringCount())
        .arg(avgDaysLeft(), 0, 'f', 1));
}

void PaperCertRenewer::loadSettings() {
    settings_.beginGroup("CertRenewer");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CertRenewerEntry e;
        e.id = settings_.value("id").toInt();
        e.domain = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.issuer = settings_.value("issuer").toString();
        e.daysLeft = settings_.value("daysLeft").toReal();
        e.renewals = settings_.value("renewals").toInt();
        e.expiring = settings_.value("expiring").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperCertRenewer::saveSettings() {
    settings_.beginGroup("CertRenewer");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("domain", entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("issuer", entries_[i].issuer);
        settings_.setValue("daysLeft", entries_[i].daysLeft);
        settings_.setValue("renewals", entries_[i].renewals);
        settings_.setValue("expiring", entries_[i].expiring);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
