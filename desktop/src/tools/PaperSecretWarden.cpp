#include "tools/PaperSecretWarden.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperSecretWarden::PaperSecretWarden(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SecretWarden")
{
    setupUI();
    loadSettings();
}

void PaperSecretWarden::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "API Key", "Password", "Token", "Certificate"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    connect(categoryCombo_, &QComboBox::currentTextChanged, this, [this]() { update(); });
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter pattern...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperSecretWarden::onScan);
    toolbar->addWidget(inputField_, 2);

    scanBtn_ = new QPushButton("Scan");
    scanBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; border-radius: 4px; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(scanBtn_, &QPushButton::clicked, this, &PaperSecretWarden::onScan);
    toolbar->addWidget(scanBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSecretWarden::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Secret Warden ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 540);
}

void PaperSecretWarden::addEntry(const SecretWardenEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit secretFound(entry.id, entry.risk);
    update();
}

QList<SecretWardenEntry> PaperSecretWarden::entries() const {
    return entries_;
}

int PaperSecretWarden::exposedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.exposed) c++;
    return c;
}

qreal PaperSecretWarden::avgRisk() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.risk;
    return sum / entries_.size();
}

QMap<QString, int> PaperSecretWarden::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperSecretWarden::onScan() {
    QString pattern = inputField_->text().trimmed();
    if (pattern.isEmpty()) return;

    QStringList categories = {"API Key", "Password", "Token", "Certificate"};
    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6 API Key
        QColor(22, 163, 74),    // #16a34a Password
        QColor(124, 58, 237),   // #7c3aed Token
        QColor(217, 119, 6)     // #d97706 Certificate
    };
    QStringList severities = {"Low", "Medium", "High", "Critical"};

    int catIdx = categoryCombo_->currentIndex();
    int catSlot = (catIdx == 0)
        ? QRandomGenerator::global()->bounded(4)
        : catIdx - 1;

    qreal risk = QRandomGenerator::global()->bounded(100) / 100.0;
    int sevIdx = qMin(3, static_cast<int>(risk * 4));
    bool exposed = risk > 0.7;
    int occurrences = QRandomGenerator::global()->bounded(1, 20);

    // Build masked secret text based on pattern
    QString masked;
    int showLen = qMin(4, pattern.length());
    masked = pattern.left(showLen);
    for (int i = 0; i < 12; ++i)
        masked += QChar(0x2022); // bullet character
    masked += QString::number(QRandomGenerator::global()->bounded(1000, 9999));

    SecretWardenEntry e;
    e.id = entries_.size() + 1;
    e.secret = masked;
    e.category = categories[catSlot];
    e.severity = severities[sevIdx];
    e.risk = risk;
    e.occurrences = occurrences;
    e.exposed = exposed;
    e.color = palette[catSlot];

    addEntry(e);
    inputField_->clear();
}

void PaperSecretWarden::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperSecretWarden::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No secrets scanned");
        return;
    }

    // Filter entries by selected category
    QString selectedCategory = categoryCombo_->currentText();
    QList<SecretWardenEntry> visible;
    for (const auto& e : entries_) {
        if (selectedCategory == "All" || e.category == selectedCategory)
            visible.append(e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No secrets in this category");
        return;
    }

    int w = width(), h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Secret Warden");

    // Top half: warden view
    drawWardenView(p, QRect(20, 50, w - 40, h / 2 - 40));

    // Bottom-left: category chart
    drawCategoryChart(p, QRect(20, h / 2 + 20, w / 2 - 20, h / 2 - 50));

    // Bottom-right: stats
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSecretWarden::drawWardenView(QPainter& p, const QRect& area) {
    QString selectedCategory = categoryCombo_->currentText();
    QList<SecretWardenEntry> visible;
    for (const auto& e : entries_) {
        if (selectedCategory == "All" || e.category == selectedCategory)
            visible.append(e);
    }

    if (visible.isEmpty()) return;

    int maxShow = qMin(8, visible.size());
    int itemH = qMin(48, (area.height() - 10) / qMax(maxShow, 1));
    int rowW = area.width();

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = visible[i];
        int y = area.y() + i * (itemH + 4);
        int x = area.x();

        // Row background with rounded rect
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        QPainterPath bg;
        bg.addRoundedRect(x, y, rowW, itemH, 6, 6);
        p.drawPath(bg);

        // Left color indicator bar
        p.setBrush(e.color);
        QPainterPath bar;
        bar.addRoundedRect(x, y, 5, itemH, 2, 2);
        p.drawPath(bar);

        // Masked secret text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 9, QFont::Bold));
        p.drawText(x + 12, y + 4, rowW / 3, 18, Qt::AlignVCenter, e.secret);

        // Category label under secret
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 12, y + 22, rowW / 3, 14, Qt::AlignVCenter, e.category);

        // Severity badge
        QColor sevColor;
        if (e.severity == "Low") sevColor = QColor(34, 197, 94);
        else if (e.severity == "Medium") sevColor = QColor(234, 179, 8);
        else if (e.severity == "High") sevColor = QColor(249, 115, 22);
        else sevColor = QColor(220, 38, 38);

        int badgeX = x + rowW / 3 + 16;
        p.setPen(Qt::NoPen);
        p.setBrush(sevColor);
        QPainterPath badge;
        badge.addRoundedRect(badgeX, y + 6, 56, 16, 8, 8);
        p.drawPath(badge);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, y + 6, 56, 16, Qt::AlignCenter, e.severity);

        // Risk bar
        int riskBarX = badgeX + 66;
        int riskBarW = qMax(60, rowW / 5);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(riskBarX, y + 4, 30, 12, Qt::AlignVCenter, "Risk");
        // Track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        QPainterPath track;
        track.addRoundedRect(riskBarX + 30, y + 7, riskBarW, 8, 4, 4);
        p.drawPath(track);
        // Fill
        int fillW = static_cast<int>(e.risk * riskBarW);
        if (fillW > 0) {
            p.setBrush(sevColor);
            QPainterPath fill;
            fill.addRoundedRect(riskBarX + 30, y + 7, fillW, 8, 4, 4);
            p.drawPath(fill);
        }
        // Risk percentage
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(riskBarX, y + 20, riskBarW + 30, 12, Qt::AlignVCenter,
                   QString::number(static_cast<int>(e.risk * 100)) + "%");

        // Occurrences count
        int occX = riskBarX + riskBarW + 44;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(occX, y + 4, 40, 16, Qt::AlignVCenter, QString::number(e.occurrences) + "x");
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(occX, y + 20, 40, 12, Qt::AlignVCenter, "occurrences");

        // Exposed warning
        if (e.exposed) {
            int warnX = occX + 50;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(254, 226, 226));
            QPainterPath warnBg;
            warnBg.addRoundedRect(warnX, y + 6, 64, 16, 8, 8);
            p.drawPath(warnBg);
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(warnX, y + 6, 64, 16, Qt::AlignCenter, "EXPOSED");
        }
    }
}

void PaperSecretWarden::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"API Key", "Password", "Token", "Certificate"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(124, 58, 237),   // #7c3aed
        QColor(217, 119, 6)     // #d97706
    };

    int maxVal = 1;
    for (const auto& cat : categories) {
        if (counts.contains(cat))
            maxVal = qMax(maxVal, counts[cat]);
    }

    int barH = qMin(24, (rect.height() - 30) / 4);
    int maxBarW = rect.width() - 120;

    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 4, 70, barH, Qt::AlignRight | Qt::AlignVCenter,
                   categories[i]);

        // Bar
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * maxBarW);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        QPainterPath barPath;
        barPath.addRoundedRect(rect.x() + 76, y, qMax(barW, 4), barH - 2, 4, 4);
        p.drawPath(barPath);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 80 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperSecretWarden::drawStats(QPainter& p, const QRect& rect) {
    int expCount = exposedCount();
    qreal avg = avgRisk();
    int avgPct = static_cast<int>(avg * 100);

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Secrets",  QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Risk",       QString::number(avgPct) + "%",    QColor(217, 119, 6)},
        {"Exposed",        QString::number(expCount),        QColor(220, 38, 38)},
        {"Categories",     QString::number(categoryCounts().size()), QColor(124, 58, 237)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath box;
        box.addRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.drawPath(box);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperSecretWarden::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Secret Warden ready");
        return;
    }
    int exp = exposedCount();
    int avg = static_cast<int>(avgRisk() * 100);
    infoLabel_->setText(
        QString("%1 secrets | %2 exposed | avg risk %3% | %4 categories")
            .arg(entries_.size())
            .arg(exp)
            .arg(avg)
            .arg(categoryCounts().size()));
}

void PaperSecretWarden::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SecretWardenEntry e;
        e.id = settings_.value("id").toInt();
        e.secret = settings_.value("secret").toString();
        e.category = settings_.value("category").toString();
        e.severity = settings_.value("severity").toString();
        e.risk = settings_.value("risk").toReal();
        e.occurrences = settings_.value("occurrences").toInt();
        e.exposed = settings_.value("exposed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSecretWarden::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("secret", entries_[i].secret);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("risk", entries_[i].risk);
        settings_.setValue("occurrences", entries_[i].occurrences);
        settings_.setValue("exposed", entries_[i].exposed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
