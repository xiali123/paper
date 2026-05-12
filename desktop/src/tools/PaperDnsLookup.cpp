#include "tools/PaperDnsLookup.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperDnsLookup::PaperDnsLookup(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DnsLookup")
{
    setupUI();
    loadSettings();
}

void PaperDnsLookup::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "A", "AAAA", "CNAME", "MX"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 80px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter domain...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; }");
    toolbar->addWidget(inputField_, 1);

    lookupBtn_ = new QPushButton("Lookup");
    lookupBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(lookupBtn_, &QPushButton::clicked, this, &PaperDnsLookup::onLookup);
    toolbar->addWidget(lookupBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; background: transparent; border: 1px solid #dc2626; "
        "padding: 5px 12px; border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDnsLookup::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("DNS Lookup");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 2px 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch(1);

    setMinimumSize(620, 520);
}

void PaperDnsLookup::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f8fafc"));

    int w = width();
    int h = height();

    // Compute control bar height: toolbar + info label + margins
    int controlH = 80;
    int contentY = controlH;
    int contentH = h - contentY - 8;

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(QRect(0, contentY, w, contentH), Qt::AlignCenter, "No DNS entries - enter a domain and click Lookup");
        return;
    }

    // Top half: lookup view
    int lookupH = contentH * 55 / 100;
    drawLookupView(p, QRect(10, contentY, w - 20, lookupH));

    // Bottom half split into left chart and right stats
    int bottomY = contentY + lookupH + 8;
    int bottomH = contentH - lookupH - 8;
    int halfW = (w - 28) / 2;

    drawCategoryChart(p, QRect(10, bottomY, halfW, bottomH));
    drawStats(p, QRect(10 + halfW + 8, bottomY, halfW, bottomH));
}

void PaperDnsLookup::drawLookupView(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 22, Qt::AlignLeft | Qt::AlignTop, "DNS Lookup Results");

    // Filter by category
    QString filter = categoryCombo_->currentText();

    // Collect visible entries
    QList<const DnsLookupEntry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter) {
            visible.append(&e);
        }
    }

    int maxShow = qMin(8, visible.size());
    if (maxShow == 0) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   "No entries for category: " + filter);
        return;
    }

    int rowH = qMin(36, (rect.height() - 34) / maxShow);

    // Category color map
    auto catColor = [](const QString& cat) -> QColor {
        if (cat == "A")     return QColor("#3b82f6");
        if (cat == "AAAA")  return QColor("#16a34a");
        if (cat == "CNAME") return QColor("#7c3aed");
        if (cat == "MX")    return QColor("#d97706");
        return QColor("#64748b");
    };

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = *visible[i];
        int y = rect.y() + 28 + i * (rowH + 3);
        QColor catClr = catColor(e.category);

        // Row background with QPainterPath rounded rect
        QPainterPath rowBg;
        rowBg.addRoundedRect(rect.x(), y, rect.width(), rowH, 5, 5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawPath(rowBg);

        // Left color accent bar
        QPainterPath accent;
        accent.addRoundedRect(rect.x(), y, 4, rowH, 2, 2);
        p.setBrush(catClr);
        p.drawPath(accent);

        int textX = rect.x() + 12;

        // Domain name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 9, QFont::Bold));
        QString displayDomain = e.domain.length() > 24 ? e.domain.left(22) + ".." : e.domain;
        p.drawText(textX, y + 2, rect.width() / 3, 16, Qt::AlignVCenter | Qt::AlignLeft, displayDomain);

        // Record type badge
        QColor badgeBg = catClr;
        int badgeX = textX + rect.width() / 3 + 4;
        QPainterPath badge;
        badge.addRoundedRect(badgeX, y + 3, 40, 14, 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(badgeBg);
        p.drawPath(badge);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, y + 3, 40, 14, Qt::AlignCenter, e.record);

        // Latency bar
        int barMaxW = rect.width() / 4;
        int barW = qBound(4, static_cast<int>((e.latency / 300.0) * barMaxW), barMaxW);
        QColor latencyClr = e.latency < 30 ? QColor("#16a34a")
                          : e.latency < 100 ? QColor("#d97706")
                          : QColor("#dc2626");
        int barX = badgeX + 48;
        QPainterPath latBar;
        latBar.addRoundedRect(barX, y + rowH / 2 - 3, barW, 6, 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(latencyClr);
        p.drawPath(latBar);

        // Latency text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, y + rowH / 2 + 4,
                   QString::number(static_cast<int>(e.latency)) + "ms");

        // Records count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        int recX = barX + barMaxW + 50;
        p.drawText(recX, y + rowH / 2 + 4, QString::number(e.records) + " records");

        // Resolved indicator
        if (e.resolved) {
            QPainterPath check;
            int cx = rect.x() + rect.width() - 22;
            check.addEllipse(cx, y + rowH / 2 - 7, 14, 14);
            p.setBrush(QColor("#16a34a"));
            p.setPen(Qt::NoPen);
            p.drawPath(check);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(cx, y + rowH / 2 - 7, 14, 14, Qt::AlignCenter, QString::fromUtf8("✓"));
        } else {
            QPainterPath cross;
            int cx = rect.x() + rect.width() - 22;
            cross.addEllipse(cx, y + rowH / 2 - 7, 14, 14);
            p.setBrush(QColor("#dc2626"));
            p.setPen(Qt::NoPen);
            p.drawPath(cross);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(cx, y + rowH / 2 - 7, 14, 14, Qt::AlignCenter, QString::fromUtf8("✗"));
        }
    }
}

void PaperDnsLookup::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 20, Qt::AlignLeft | Qt::AlignTop, "Category Distribution");

    auto counts = categoryCounts();
    QStringList cats = {"A", "AAAA", "CNAME", "MX"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#7c3aed"), QColor("#d97706")
    };

    int maxVal = 1;
    for (const auto& cat : cats) {
        if (counts.contains(cat)) maxVal = qMax(maxVal, counts[cat]);
    }

    int barH = qMin(24, (rect.height() - 30) / cats.size());
    int chartX = rect.x() + 50;
    int chartW = rect.width() - 70;

    for (int i = 0; i < cats.size(); ++i) {
        int y = rect.y() + 26 + i * (barH + 6);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * chartW);
        barW = qMax(barW, 0);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 46, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        // Bar with QPainterPath
        if (barW > 0) {
            QPainterPath bar;
            bar.addRoundedRect(chartX, y + 2, barW, barH - 4, 3, 3);
            p.setPen(Qt::NoPen);
            p.setBrush(colors[i]);
            p.drawPath(bar);
        }

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(chartX + barW + 6, y + barH / 2 + 3, QString::number(count));
    }
}

void PaperDnsLookup::drawStats(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 20, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int resolved = resolvedCount();
    qreal avg = avgLatency();

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Lookups", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Resolved",      QString::number(resolved),        QColor("#16a34a")},
        {"Avg Latency",   QString::number(avg, 'f', 1) + "ms", QColor("#d97706")},
        {"Categories",    QString::number(categoryCounts().size()), QColor("#7c3aed")}
    };

    int boxH = qMin(38, (rect.height() - 30) / stats.size());

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 26 + i * (boxH + 5);

        // Background with QPainterPath
        QPainterPath box;
        box.addRoundedRect(rect.x(), y, rect.width(), boxH, 5, 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(box);

        // Left accent
        QPainterPath accent;
        accent.addRoundedRect(rect.x(), y, 3, boxH, 1, 1);
        p.setBrush(stats[i].color);
        p.drawPath(accent);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(rect.x() + 12, y + 2, rect.width() - 20, boxH / 2, Qt::AlignVCenter | Qt::AlignLeft, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 12, y + boxH / 2, rect.width() - 20, boxH / 2, Qt::AlignVCenter | Qt::AlignLeft, stats[i].label);
    }
}

void PaperDnsLookup::onLookup() {
    QString domain = inputField_->text().trimmed();
    if (domain.isEmpty()) return;

    QString category;
    int idx = categoryCombo_->currentIndex();
    if (idx == 0) {
        // "All" selected: pick a random category
        QStringList cats = {"A", "AAAA", "CNAME", "MX"};
        category = cats[QRandomGenerator::global()->bounded(cats.size())];
    } else {
        QStringList cats = {"All", "A", "AAAA", "CNAME", "MX"};
        category = cats[idx];
    }

    QColor catColor;
    if (category == "A")     catColor = QColor("#3b82f6");
    else if (category == "AAAA")  catColor = QColor("#16a34a");
    else if (category == "CNAME") catColor = QColor("#7c3aed");
    else if (category == "MX")    catColor = QColor("#d97706");
    else                          catColor = QColor("#64748b");

    QStringList recordTypes = {"A", "AAAA", "CNAME", "MX"};
    int numRecords = 1 + QRandomGenerator::global()->bounded(4);

    for (int i = 0; i < numRecords; ++i) {
        DnsLookupEntry entry;
        entry.id = entries_.size() + 1;
        entry.domain = domain;
        entry.category = category;
        entry.record = recordTypes[QRandomGenerator::global()->bounded(recordTypes.size())];
        entry.latency = 2 + QRandomGenerator::global()->boundedDouble() * 280.0;
        entry.records = 1 + QRandomGenerator::global()->bounded(12);
        entry.resolved = QRandomGenerator::global()->bounded(10) < 7; // 70% success rate
        entry.color = catColor;

        entries_.append(entry);
        emit dnsResolved(entry.id, entry.latency);
    }

    inputField_->clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperDnsLookup::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperDnsLookup::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("DNS Lookup - enter a domain to begin");
        return;
    }
    int resolved = resolvedCount();
    qreal avg = avgLatency();
    infoLabel_->setText(
        QString("Lookups: %1 | Resolved: %2 | Avg Latency: %3ms | Categories: %4")
            .arg(entries_.size())
            .arg(resolved)
            .arg(avg, 0, 'f', 1)
            .arg(categoryCounts().size()));
}

void PaperDnsLookup::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DnsLookupEntry e;
        e.id = settings_.value("id").toInt();
        e.domain = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.record = settings_.value("record").toString();
        e.latency = settings_.value("latency").toDouble();
        e.records = settings_.value("records").toInt();
        e.resolved = settings_.value("resolved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDnsLookup::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("domain", entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("record", entries_[i].record);
        settings_.setValue("latency", entries_[i].latency);
        settings_.setValue("records", entries_[i].records);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}

QList<DnsLookupEntry> PaperDnsLookup::entries() const {
    return entries_;
}

int PaperDnsLookup::resolvedCount() const {
    int c = 0;
    for (const auto& e : entries_) {
        if (e.resolved) ++c;
    }
    return c;
}

qreal PaperDnsLookup::avgLatency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.latency;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperDnsLookup::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}
