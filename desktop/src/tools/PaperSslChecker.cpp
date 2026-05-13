#include "tools/PaperSslChecker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperSslChecker::PaperSslChecker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SslChecker")
{
    setupUI();
    loadSettings();

    // Seed 8 entries across 5 categories
    if (entries_.isEmpty()) {
        struct Seed { QString domain; QString category; QString protocol; };
        Seed seeds[] = {
            {"papers.example.com",    "Production",  "TLS 1.3"},
            {"staging.paper.dev",     "Staging",     "TLS 1.2"},
            {"dev.local",             "Development", "TLS 1.2"},
            {"api.scholar.io",        "API",         "TLS 1.3"},
            {"cdn.assets.net",        "CDN",         "TLS 1.3"},
            {"auth.research.org",     "Production",  "TLS 1.3"},
            {"test.paper.dev",        "Staging",     "TLS 1.2"},
            {"gateway.svc.local",     "API",         "TLS 1.2"},
        };

        QMap<QString, QColor> catColor;
        catColor["Production"]  = QColor(59, 130, 246);   // #3b82f6
        catColor["Staging"]     = QColor(22, 163, 74);    // #16a34a
        catColor["Development"] = QColor(217, 119, 6);    // #d97706
        catColor["API"]         = QColor(220, 38, 38);    // #dc2626
        catColor["CDN"]         = QColor(124, 58, 237);   // #7c3aed

        for (int i = 0; i < 8; ++i) {
            SslCheckerEntry e;
            e.id       = i + 1;
            e.domain   = seeds[i].domain;
            e.category = seeds[i].category;
            e.protocol = seeds[i].protocol;
            e.score    = 60.0 + QRandomGenerator::global()->bounded(40);
            e.days     = 10 + QRandomGenerator::global()->bounded(350);
            e.valid    = e.score >= 70.0 && e.days > 30;
            e.color    = catColor.value(e.category, QColor(107, 114, 128));

            // Urgency override
            if (e.days <= 14)
                e.color = QColor(220, 38, 38);
            else if (e.days <= 30)
                e.color = QColor(217, 119, 6);

            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperSslChecker::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Top toolbar row
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Production", "Staging", "Development", "API", "CDN"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 110px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter domain to check...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperSslChecker::onCheck);
    toolbar->addWidget(checkBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 4px 12px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSslChecker::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel("Paper SSL Checker");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 2px 4px;");
    mainLayout->addWidget(infoLabel_);

    // Canvas area for custom painting
    mainLayout->addStretch(1);

    setMinimumSize(680, 520);
}

void PaperSslChecker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    int toolbarH = 90;
    int canvasY  = toolbarH;
    int w = width() - 16;
    int h = height() - canvasY - 8;

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(QRect(8, canvasY, w, h), Qt::AlignCenter,
                   "No SSL entries - enter a domain and click Check");
        return;
    }

    // Top half: checker view
    int halfH = h * 55 / 100;
    drawCheckerView(p, QRect(8, canvasY, w, halfH));

    // Bottom half split: left = category chart, right = stats
    int bottomY = canvasY + halfH + 6;
    int bottomH = h - halfH - 6;
    int halfW   = w / 2;

    drawCategoryChart(p, QRect(8, bottomY, halfW - 4, bottomH));
    drawStats(p, QRect(8 + halfW + 4, bottomY, halfW - 4, bottomH));
}

void PaperSslChecker::drawCheckerView(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(area.x() + 4, area.y() + 18, "SSL/TLS Checker View");

    // Background panel
    QPainterPath panelPath;
    panelPath.addRoundedRect(area.x(), area.y() + 26, area.width(), area.height() - 26, 8, 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawPath(panelPath);

    int listY   = area.y() + 34;
    int maxShow = qMin(static_cast<int>(entries_.size()), 10);
    int availH  = area.height() - 42;
    int itemH   = qMin(48, availH / qMax(maxShow, 1));
    int barMaxW = area.width() - 160;

    int shown = 0;
    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        int y   = listY + shown * (itemH + 4);
        int rowW = area.width() - 8;
        int x   = area.x() + 4;

        // Row background
        QPainterPath rowPath;
        rowPath.addRoundedRect(x, y, rowW, itemH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawPath(rowPath);

        // Left accent bar
        QPainterPath accentPath;
        accentPath.addRoundedRect(x, y, 4, itemH, 2, 2);
        p.setBrush(e.color);
        p.drawPath(accentPath);

        // Domain name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(x + 14, y + 3, rowW - 140, 18, Qt::AlignVCenter,
                   e.domain.left(32));

        // Protocol + category line
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 14, y + 20, rowW - 140, 14, Qt::AlignVCenter,
                   e.protocol + " | " + e.category + " | Score: "
                   + QString::number(static_cast<int>(e.score)));

        // Days-remaining progress bar
        int barH = 6;
        int barY = y + itemH - 12;
        int barX = x + 14;
        int filledW = static_cast<int>((qBound(0.0, static_cast<qreal>(e.days), 365.0) / 365.0) * barMaxW);

        // Bar track
        QPainterPath trackPath;
        trackPath.addRoundedRect(barX, barY, barMaxW, barH, 3, 3);
        p.setBrush(QColor(226, 232, 240));
        p.drawPath(trackPath);

        // Bar fill
        if (filledW > 0) {
            QPainterPath fillPath;
            fillPath.addRoundedRect(barX, barY, qMax(filledW, 4), barH, 3, 3);
            p.setBrush(e.color);
            p.drawPath(fillPath);
        }

        // Days label next to bar
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barMaxW + 6, barY + barH - 1,
                   QString::number(e.days) + "d");

        // Valid / Invalid badge
        int badgeW = 56;
        int badgeH = 20;
        int badgeX = x + rowW - badgeW - 6;
        int badgeY = y + (itemH - badgeH) / 2;

        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 10, 10);
        p.setPen(Qt::NoPen);
        p.setBrush(e.valid ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.drawPath(badgePath);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter,
                   e.valid ? "VALID" : "INVALID");

        ++shown;
    }
}

void PaperSslChecker::drawCategoryChart(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 4, area.y() + 16, "Category Breakdown");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    // Background panel
    QPainterPath panelPath;
    panelPath.addRoundedRect(area.x(), area.y() + 22, area.width(), area.height() - 22, 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawPath(panelPath);

    int maxVal = 1;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxVal = qMax(maxVal, it.value());

    // Category color map
    QMap<QString, QColor> catColor;
    catColor["Production"]  = QColor(59, 130, 246);
    catColor["Staging"]     = QColor(22, 163, 74);
    catColor["Development"] = QColor(217, 119, 6);
    catColor["API"]         = QColor(220, 38, 38);
    catColor["CDN"]         = QColor(124, 58, 237);

    int nCats     = counts.size();
    int listStartY = area.y() + 30;
    int barH      = qMin(26, (area.height() - 40) / qMax(nCats, 1));
    int maxBarW   = area.width() - 120;

    int i = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y     = listStartY + i * (barH + 5);
        int count = it.value();
        int barW  = static_cast<int>((static_cast<qreal>(count) / maxVal) * maxBarW);

        QColor color = catColor.value(it.key(), QColor(107, 114, 128));

        // Category label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(area.x() + 8, y, 80, barH, Qt::AlignVCenter | Qt::AlignRight,
                   it.key());

        // Bar
        QPainterPath barPath;
        barPath.addRoundedRect(area.x() + 90, y + 2, qMax(barW, 4), barH - 4, 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(barPath);

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + 96 + barW, y, 30, barH, Qt::AlignVCenter,
                   QString::number(count));

        ++i;
    }
}

void PaperSslChecker::drawStats(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(area.x() + 4, area.y() + 16, "Statistics");

    // Background panel
    QPainterPath panelPath;
    panelPath.addRoundedRect(area.x(), area.y() + 22, area.width(), area.height() - 22, 6, 6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawPath(panelPath);

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Entries",  QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Score",      QString::number(avgScore(), 'f', 1), QColor(217, 119, 6)},
        {"Valid Count",    QString::number(validCount()),    QColor(22, 163, 74)},
    };

    int boxH    = qMin(52, (area.height() - 36) / qMax(stats.size(), 1));
    int boxW    = area.width() - 16;
    int startX  = area.x() + 8;
    int startY  = area.y() + 30;

    for (int i = 0; i < stats.size(); ++i) {
        int y = startY + i * (boxH + 5);

        // Stat box background
        QPainterPath boxPath;
        boxPath.addRoundedRect(startX, y, boxW, boxH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(boxPath);

        // Left accent
        QPainterPath accentPath;
        accentPath.addRoundedRect(startX, y, 4, boxH, 2, 2);
        p.setBrush(stats[i].color);
        p.drawPath(accentPath);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(startX + 14, y + 2, boxW - 20, 26, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(startX + 14, y + 28, boxW - 20, 18, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperSslChecker::onCheck() {
    QString domain = inputField_->text().trimmed();
    if (domain.isEmpty()) return;

    // Resolve category from combo
    QString category = categoryCombo_->currentText();
    if (category == "All")
        category = "Production";

    QStringList protocols = {"TLS 1.2", "TLS 1.3"};
    QString protocol = protocols.at(QRandomGenerator::global()->bounded(protocols.size()));

    qreal score = 60.0 + QRandomGenerator::global()->bounded(40);
    int days    = 10 + QRandomGenerator::global()->bounded(350);
    bool valid  = score >= 70.0 && days > 30;

    // Category-based color assignment
    QMap<QString, QColor> catColor;
    catColor["Production"]  = QColor(59, 130, 246);
    catColor["Staging"]     = QColor(22, 163, 74);
    catColor["Development"] = QColor(217, 119, 6);
    catColor["API"]         = QColor(220, 38, 38);
    catColor["CDN"]         = QColor(124, 58, 237);

    QColor color = catColor.value(category, QColor(107, 114, 128));
    // Urgency override
    if (days <= 14)
        color = QColor(220, 38, 38);
    else if (days <= 30)
        color = QColor(217, 119, 6);

    SslCheckerEntry entry;
    entry.id       = entries_.size() + 1;
    entry.domain   = domain;
    entry.category = category;
    entry.protocol = protocol;
    entry.score    = score;
    entry.days     = days;
    entry.valid    = valid;
    entry.color    = color;

    addEntry(entry);
    inputField_->clear();
}

void PaperSslChecker::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperSslChecker::addEntry(const SslCheckerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit certChecked(entry.id, entry.score);
    update();
}

QList<SslCheckerEntry> PaperSslChecker::entries() const {
    return entries_;
}

int PaperSslChecker::validCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.valid) ++count;
    return count;
}

qreal PaperSslChecker::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.score;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperSslChecker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSslChecker::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Paper SSL Checker");
        return;
    }
    infoLabel_->setText(
        QString("Entries: %1 | Valid: %2 | Avg Score: %3")
            .arg(entries_.size())
            .arg(validCount())
            .arg(avgScore(), 0, 'f', 1));
}

void PaperSslChecker::loadSettings() {
    settings_.beginGroup("SslChecker");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SslCheckerEntry e;
        e.id       = settings_.value("id").toInt();
        e.domain   = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.protocol = settings_.value("protocol").toString();
        e.score    = settings_.value("score").toReal();
        e.days     = settings_.value("days").toInt();
        e.valid    = settings_.value("valid").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperSslChecker::saveSettings() {
    settings_.beginGroup("SslChecker");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("domain",   entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("protocol", entries_[i].protocol);
        settings_.setValue("score",    entries_[i].score);
        settings_.setValue("days",     entries_[i].days);
        settings_.setValue("valid",    entries_[i].valid);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
