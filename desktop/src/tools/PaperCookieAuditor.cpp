#include "tools/PaperCookieAuditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperCookieAuditor::PaperCookieAuditor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CookieAuditor")
{
    setupUI();
    loadSettings();
}

void PaperCookieAuditor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Essential", "Analytics", "Marketing", "Functional", "Third-Party"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Domain filter...");
    toolbar->addWidget(inputField_, 1);

    auditBtn_ = new QPushButton("Audit");
    auditBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(auditBtn_, &QPushButton::clicked, this, &PaperCookieAuditor::onAudit);
    toolbar->addWidget(auditBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCookieAuditor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Cookie auditor ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 480);
}

void PaperCookieAuditor::addEntry(const CookieAuditorEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit cookieAudited(entry.id, entry.expiry);
    update();
}

QList<CookieAuditorEntry> PaperCookieAuditor::entries() const {
    return entries_;
}

int PaperCookieAuditor::secureCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.secure) c++;
    return c;
}

qreal PaperCookieAuditor::avgExpiry() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_)
        total += e.expiry;
    return total / entries_.size();
}

QMap<QString, int> PaperCookieAuditor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperCookieAuditor::onAudit() {
    static const QStringList categories = {"Essential", "Analytics", "Marketing", "Functional", "Third-Party"};
    static const QList<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    if (entries_.isEmpty()) {
        struct SeedData {
            QString domain; QString category; QString cookie;
            qreal expiry; int count; bool secure;
        };
        SeedData seeds[] = {
            {"example.com",        "Essential",  "session_id",    24.0,  1, true},
            {"analytics.io",       "Analytics",  "tracker_uid",   720.0, 3, false},
            {"ads.marketing.net",  "Marketing",  "ad_pref",       4320.0, 5, false},
            {"cdn.functional.org", "Functional", "lang_pref",     168.0, 2, true},
            {"tracker.3rdparty.com", "Third-Party", "ext_id",     8760.0, 8, false},
            {"login.example.com",  "Essential",  "auth_token",    48.0,  1, true},
            {"stats.analytics.io", "Analytics",  "page_view",     360.0, 12, false},
            {"pixel.ads.net",      "Marketing",  "retarget",      2160.0, 4, false},
        };

        for (int i = 0; i < 8; ++i) {
            CookieAuditorEntry e;
            e.id = i + 1;
            e.domain = seeds[i].domain;
            e.category = seeds[i].category;
            e.cookie = seeds[i].cookie;
            e.expiry = seeds[i].expiry;
            e.count = seeds[i].count;
            e.secure = seeds[i].secure;
            int catIdx = categories.indexOf(e.category);
            e.color = palette[qBound(0, catIdx, palette.size() - 1)];
            entries_.append(e);
        }
    } else {
        int nextId = entries_.last().id + 1;
        int catIdx = QRandomGenerator::global()->bounded(categories.size());
        CookieAuditorEntry e;
        e.id = nextId;
        e.category = categories[catIdx];
        e.color = palette[catIdx];
        e.domain = QString("cookie%1.local").arg(nextId);
        e.cookie = QString("ck_%1").arg(nextId);
        e.expiry = static_cast<qreal>(QRandomGenerator::global()->bounded(1, 8761));
        e.count = QRandomGenerator::global()->bounded(1, 16);
        e.secure = QRandomGenerator::global()->bounded(2) == 1;
        entries_.append(e);
        emit cookieAudited(e.id, e.expiry);
    }

    saveSettings();
    updateInfo();
    update();
}

void PaperCookieAuditor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Cookie auditor ready");
    update();
}

void PaperCookieAuditor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Click Audit to scan cookies");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Cookie Audit Report");

    int w = width(), h = height();
    drawAuditView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCookieAuditor::drawAuditView(QPainter& p, const QRect& rect) {
    QString filterCat = categoryCombo_->currentText();
    QString domainFilter = inputField_->text().trimmed().toLower();

    int show = 0;
    int maxShow = 10;
    int itemH = qMin(40, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];

        if (filterCat != "All" && e.category != filterCat) continue;
        if (!domainFilter.isEmpty() && !e.domain.toLower().contains(domainFilter)) continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 4, y + 4, 6, itemH - 8, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 16, y + 2, rect.width() - 70, 16, Qt::AlignVCenter,
                   e.domain.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 16, y + 18, rect.width() - 70, 14, Qt::AlignVCenter,
                   e.cookie + " | " + e.category);

        QString secureTag = e.secure ? "Secure" : "Insecure";
        p.setPen(e.secure ? QColor("#16a34a") : QColor("#dc2626"));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 56, y + 4, 52, 14,
                   Qt::AlignVCenter | Qt::AlignRight, secureTag);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 56, y + 20, 52, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString("%1h | x%2").arg(static_cast<int>(e.expiry)).arg(e.count));
        show++;
    }

    if (show == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching cookies");
    }
}

void PaperCookieAuditor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = {"Essential", "Analytics", "Marketing", "Functional", "Third-Party"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 72, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 78, y, qMax(barW, 2), barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 82 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperCookieAuditor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Cookies", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Secure", QString::number(secureCount()), QColor("#16a34a")},
        {"Avg Expiry (h)", QString::number(avgExpiry(), 'f', 1), QColor("#d97706")},
        {"Categories", QString::number(categoryCounts().size()), QColor("#7c3aed")}
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

void PaperCookieAuditor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Cookie auditor ready"); return; }
    infoLabel_->setText(QString("%1 cookies | %2 secure | avg expiry %3h")
        .arg(entries_.size()).arg(secureCount())
        .arg(QString::number(avgExpiry(), 'f', 1)));
}

void PaperCookieAuditor::loadSettings() {
    int size = settings_.beginReadArray("cookies");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CookieAuditorEntry e;
        e.id = settings_.value("id").toInt();
        e.domain = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.cookie = settings_.value("cookie").toString();
        e.expiry = settings_.value("expiry").toReal();
        e.count = settings_.value("count").toInt();
        e.secure = settings_.value("secure").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCookieAuditor::saveSettings() {
    settings_.beginWriteArray("cookies");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("domain", entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("cookie", entries_[i].cookie);
        settings_.setValue("expiry", entries_[i].expiry);
        settings_.setValue("count", entries_[i].count);
        settings_.setValue("secure", entries_[i].secure);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
