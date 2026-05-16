#include "tools/PaperSecretWarden2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

namespace {
const QStringList kVaults = {"Production", "Staging", "Dev", "CI-CD"};
const QStringList kCategories = {"API Keys", "Tokens", "Certificates", "Passwords", "OAuth"};
const QColor kPalette[] = {
    QColor(59, 130, 246),   // #3b82f6
    QColor(22, 163, 74),    // #16a34a
    QColor(217, 119, 6),    // #d97706
    QColor(220, 38, 38),    // #dc2626
    QColor(124, 58, 237)    // #7c3aed
};

QColor vaultColor(const QString& vault) {
    int idx = kVaults.indexOf(vault);
    if (idx < 0) idx = 0;
    return kPalette[idx % 5];
}

QColor categoryColor(const QString& category) {
    int idx = kCategories.indexOf(category);
    if (idx < 0) idx = 0;
    return kPalette[idx % 5];
}
} // anonymous namespace

PaperSecretWarden2::PaperSecretWarden2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SecretWarden2")
{
    setupUI();
    loadSettings();
}

void PaperSecretWarden2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "API Keys", "Tokens", "Certificates", "Passwords", "OAuth"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    connect(categoryCombo_, &QComboBox::currentTextChanged, this, [this]() { update(); });
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter secret name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperSecretWarden2::onScan);
    toolbar->addWidget(inputField_, 2);

    scanBtn_ = new QPushButton("Scan");
    scanBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; border-radius: 4px; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(scanBtn_, &QPushButton::clicked, this, &PaperSecretWarden2::onScan);
    toolbar->addWidget(scanBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSecretWarden2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Secret Warden 2 ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 580);
}

// --- seed data ---
static void seedEntries(QList<SecretWarden2Entry>& entries) {
    struct Seed {
        const char* secret; const char* category; const char* vault;
        qreal rotation; int accesses; bool expired;
    };
    Seed seeds[] = {
        {"sk-prod-ak-3f8a••••9201",  "API Keys",      "Production", 45.0, 127, true },
        {"ghp-stg-tk-7c2d••••5843",  "Tokens",         "Staging",    12.0, 43,  false},
        {"cert-dev-ssl-a1b2••••e4f5","Certificates",   "Dev",         7.0, 18,  false},
        {"pwd-cicd-deploy-x9y8••••z0","Passwords",     "CI-CD",      90.0, 312, true },
        {"oauth-prod-google-k1l2••••m3n4","OAuth",      "Production", 30.0, 89,  false},
        {"sk-stg-ak-5e6f••••7890",   "API Keys",       "Staging",    60.0, 67,  true },
        {"tok-dev-refresh-p1q2••••r3s4","Tokens",       "Dev",         3.0, 5,   false},
        {"cert-cicd-wildcard-t5u6••••v7w8","Certificates","CI-CD",    120.0, 201, true },
    };

    QColor seedColors[] = {
        QColor(59, 130, 246),  // API Keys -> blue
        QColor(124, 58, 237),  // Tokens -> purple
        QColor(217, 119, 6),   // Certificates -> amber
        QColor(220, 38, 38),   // Passwords -> red
        QColor(124, 58, 237),  // OAuth -> purple
        QColor(59, 130, 246),  // API Keys -> blue
        QColor(124, 58, 237),  // Tokens -> purple
        QColor(217, 119, 6),   // Certificates -> amber
    };

    for (int i = 0; i < 8; ++i) {
        const auto& s = seeds[i];
        SecretWarden2Entry e;
        e.id = i + 1;
        e.secret = s.secret;
        e.category = s.category;
        e.vault = s.vault;
        e.rotation = s.rotation;
        e.accesses = s.accesses;
        e.expired = s.expired;
        e.color = seedColors[i];
        entries.append(e);
    }
}

void PaperSecretWarden2::addEntry(const SecretWarden2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit secretRotated(entry.id, entry.rotation);
    update();
}

QList<SecretWarden2Entry> PaperSecretWarden2::entries() const {
    return entries_;
}

int PaperSecretWarden2::expiredCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.expired) ++c;
    return c;
}

qreal PaperSecretWarden2::avgRotation() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.rotation;
    return sum / entries_.size();
}

QMap<QString, int> PaperSecretWarden2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperSecretWarden2::onScan() {
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    int catIdx = categoryCombo_->currentIndex();
    int catSlot = (catIdx == 0)
        ? QRandomGenerator::global()->bounded(kCategories.size())
        : catIdx - 1;
    int vaultSlot = QRandomGenerator::global()->bounded(kVaults.size());

    // Build masked secret
    QString masked;
    int showLen = qMin(4, name.length());
    masked = name.left(showLen);
    for (int i = 0; i < 8; ++i)
        masked += QChar(0x2022);
    masked += QString::number(QRandomGenerator::global()->bounded(1000, 9999));

    qreal rotation = QRandomGenerator::global()->bounded(180);
    int accesses = QRandomGenerator::global()->bounded(1, 300);
    bool expired = rotation > 90.0;

    SecretWarden2Entry e;
    e.id = entries_.size() + 1;
    e.secret = masked;
    e.category = kCategories[catSlot];
    e.vault = kVaults[vaultSlot];
    e.rotation = rotation;
    e.accesses = accesses;
    e.expired = expired;
    e.color = categoryColor(kCategories[catSlot]);

    addEntry(e);
    inputField_->clear();
}

void PaperSecretWarden2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// --- painting ---

void PaperSecretWarden2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No secrets scanned -- press Scan to seed");
        return;
    }

    // Filter by selected category
    QString selectedCategory = categoryCombo_->currentText();
    QList<SecretWarden2Entry> visible;
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
    p.drawText(20, 30, "Secret Warden 2");

    // Top half: warden card view
    drawWardenView(p, QRect(20, 50, w - 40, h / 2 - 40));

    // Bottom-left: category bar chart
    drawCategoryChart(p, QRect(20, h / 2 + 20, w / 2 - 20, h / 2 - 50));

    // Bottom-right: stats boxes
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSecretWarden2::drawWardenView(QPainter& p, const QRect& area) {
    QString selectedCategory = categoryCombo_->currentText();
    QList<SecretWarden2Entry> visible;
    for (const auto& e : entries_) {
        if (selectedCategory == "All" || e.category == selectedCategory)
            visible.append(e);
    }

    if (visible.isEmpty()) return;

    int maxShow = qMin(8, visible.size());
    int itemH = qMin(50, (area.height() - 10) / qMax(maxShow, 1));
    int rowW = area.width();

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = visible[i];
        int y = area.y() + i * (itemH + 4);
        int x = area.x();

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.expired ? QColor(254, 242, 242) : QColor(248, 250, 252));
        QPainterPath bg;
        bg.addRoundedRect(x, y, rowW, itemH, 6, 6);
        p.drawPath(bg);

        // Left color indicator bar
        p.setBrush(e.color);
        QPainterPath bar;
        bar.addRoundedRect(x, y, 5, itemH, 2, 2);
        p.drawPath(bar);

        // Secret text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 9, QFont::Bold));
        p.drawText(x + 12, y + 2, rowW / 3, 18, Qt::AlignVCenter, e.secret);

        // Category label under secret
        p.setPen(e.color);
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 12, y + 20, rowW / 3, 14, Qt::AlignVCenter, e.category);

        // Vault badge
        QColor vCol = vaultColor(e.vault);
        int badgeX = x + rowW / 3 + 16;
        p.setPen(Qt::NoPen);
        p.setBrush(vCol);
        QPainterPath badge;
        badge.addRoundedRect(badgeX, y + 6, e.vault.length() * 7 + 16, 16, 8, 8);
        p.drawPath(badge);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, y + 6, e.vault.length() * 7 + 16, 16, Qt::AlignCenter, e.vault);

        // Rotation bar (days since rotation)
        int rotBarX = badgeX + e.vault.length() * 7 + 28;
        int rotBarW = qMax(60, rowW / 5);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rotBarX, y + 4, 38, 12, Qt::AlignVCenter, "Rotation");
        // Track background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        QPainterPath track;
        track.addRoundedRect(rotBarX + 40, y + 7, rotBarW, 8, 4, 4);
        p.drawPath(track);
        // Rotation fill: 180 days max scale
        qreal rotPct = qMin(e.rotation / 180.0, 1.0);
        int fillW = static_cast<int>(rotPct * rotBarW);
        QColor rotCol = (e.rotation < 30) ? QColor(22, 163, 74)
                       : (e.rotation < 90) ? QColor(217, 119, 6)
                       : QColor(220, 38, 38);
        if (fillW > 0) {
            p.setBrush(rotCol);
            QPainterPath fill;
            fill.addRoundedRect(rotBarX + 40, y + 7, fillW, 8, 4, 4);
            p.drawPath(fill);
        }
        // Days label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rotBarX, y + 20, rotBarW + 40, 12, Qt::AlignVCenter,
                   QString::number(static_cast<int>(e.rotation)) + "d");

        // Access count
        int accX = rotBarX + rotBarW + 56;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(accX, y + 4, 44, 16, Qt::AlignVCenter, QString::number(e.accesses) + "x");
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(accX, y + 20, 44, 12, Qt::AlignVCenter, "accesses");

        // Expired warning
        if (e.expired) {
            int warnX = accX + 52;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(254, 226, 226));
            QPainterPath warnBg;
            warnBg.addRoundedRect(warnX, y + 6, 56, 16, 8, 8);
            p.drawPath(warnBg);
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(warnX, y + 6, 56, 16, Qt::AlignCenter, "EXPIRED");
        }
    }
}

void PaperSecretWarden2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();

    QColor colors[] = {
        QColor(59, 130, 246),   // API Keys
        QColor(124, 58, 237),   // Tokens
        QColor(217, 119, 6),    // Certificates
        QColor(220, 38, 38),    // Passwords
        QColor(124, 58, 237)    // OAuth
    };

    int maxVal = 1;
    for (int i = 0; i < kCategories.size(); ++i) {
        if (counts.contains(kCategories[i]))
            maxVal = qMax(maxVal, counts[kCategories[i]]);
    }

    int barH = qMin(22, (rect.height() - 30) / kCategories.size());
    int maxBarW = rect.width() - 120;

    for (int i = 0; i < kCategories.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 6);
        int count = counts.contains(kCategories[i]) ? counts[kCategories[i]] : 0;

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 4, 76, barH, Qt::AlignRight | Qt::AlignVCenter,
                   kCategories[i]);

        // Bar
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * maxBarW);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        QPainterPath barPath;
        barPath.addRoundedRect(rect.x() + 82, y, qMax(barW, 4), barH - 2, 4, 4);
        p.drawPath(barPath);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 86 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperSecretWarden2::drawStats(QPainter& p, const QRect& rect) {
    int expired = expiredCount();
    int avgDays = static_cast<int>(avgRotation());

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Secrets",  QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Rotation",   QString::number(avgDays) + "d",   QColor(217, 119, 6)},
        {"Expired",        QString::number(expired),          QColor(220, 38, 38)},
        {"Vaults",         QString::number(kVaults.size()),   QColor(124, 58, 237)}
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

void PaperSecretWarden2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Secret Warden 2 ready");
        return;
    }
    int exp = expiredCount();
    int avg = static_cast<int>(avgRotation());
    infoLabel_->setText(
        QString("%1 secrets | %2 expired | avg rotation %3d | %4 categories")
            .arg(entries_.size())
            .arg(exp)
            .arg(avg)
            .arg(categoryCounts().size()));
}

void PaperSecretWarden2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SecretWarden2Entry e;
        e.id = settings_.value("id").toInt();
        e.secret = settings_.value("secret").toString();
        e.category = settings_.value("category").toString();
        e.vault = settings_.value("vault").toString();
        e.rotation = settings_.value("rotation").toReal();
        e.accesses = settings_.value("accesses").toInt();
        e.expired = settings_.value("expired").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    // Seed 8 entries if no persisted data
    if (entries_.isEmpty()) {
        seedEntries(entries_);
        saveSettings();
    }

    updateInfo();
}

void PaperSecretWarden2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("secret", entries_[i].secret);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("vault", entries_[i].vault);
        settings_.setValue("rotation", entries_[i].rotation);
        settings_.setValue("accesses", entries_[i].accesses);
        settings_.setValue("expired", entries_[i].expired);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
