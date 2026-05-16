#include "tools/PaperSecretVault.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>

PaperSecretVault::PaperSecretVault(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SecretVault")
{
    setupUI();
    loadSettings();
}

void PaperSecretVault::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    storeBtn_ = new QPushButton("Store");
    storeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(storeBtn_, &QPushButton::clicked, this, &PaperSecretVault::onStore);
    toolbar->addWidget(storeBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Credential", "Token", "Key", "Certificate", "Password"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSecretVault::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter secret name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Store and manage secrets securely");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperSecretVault::addEntry(const SecretEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit secretStored(entry.id, entry.type);
    update();
}

QList<SecretEntry> PaperSecretVault::entries() const { return entries_; }

int PaperSecretVault::rotatedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.rotated) c++;
    return c;
}

int PaperSecretVault::expiredCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.expired) c++;
    return c;
}

QMap<QString, int> PaperSecretVault::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperSecretVault::onStore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"credential", "token", "key", "certificate", "password"};
    QStringList types = {"API Key", "OAuth Token", "SSH Key", "X.509 Cert", "Password", "HMAC Secret"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int cIdx = categoryCombo_->currentIndex();
    QDateTime now = QDateTime::currentDateTime();

    SecretEntry e;
    e.id = entries_.size() + 1;
    e.name = text;

    if (cIdx == 0)
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    else
        e.category = categories[cIdx - 1];

    e.type = types[QRandomGenerator::global()->bounded(types.size())];

    // Random creation date within the last 180 days
    int daysAgo = QRandomGenerator::global()->bounded(180);
    QDateTime created = now.addDays(-daysAgo);
    e.created = created.toString("yyyy-MM-dd");

    // lastUsed is between created and now
    int usedDaysAgo = QRandomGenerator::global()->bounded(daysAgo + 1);
    e.lastUsed = created.addDays(usedDaysAgo).toString("yyyy-MM-dd");

    e.rotated = QRandomGenerator::global()->bounded(3) == 0;
    e.expired = daysAgo > 90;
    e.color = palette[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
    inputField_->clear();
}

void PaperSecretVault::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Store and manage secrets securely");
    update();
}

void PaperSecretVault::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Store and manage secrets securely");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Secret Vault");

    int w = width(), h = height();
    drawSecretList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSecretVault::drawSecretList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // Background bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left accent stripe
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Name with status markers
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString nameText = e.name.left(14);
        if (e.expired) nameText += " [EXP]";
        else if (e.rotated) nameText += " [ROT]";
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter, nameText);

        // Detail line: type | category
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter, e.type + " | " + e.category);

        // Right side: created date
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight, e.created);

        // Right side: last used
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "used: " + e.lastUsed);
    }
}

void PaperSecretVault::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"credential", "token", "key", "certificate", "password"};
    QString labels[] = {"Credential", "Token", "Key", "Certificate", "Password"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperSecretVault::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Secrets",    QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Rotated",    QString::number(rotatedCount()),  QColor(0x16, 0xa3, 0x4a)},
        {"Expired",    QString::number(expiredCount()),  QColor(0xd9, 0x77, 0x06)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c, 0x3a, 0xed)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
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

void PaperSecretVault::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Store and manage secrets securely");
        return;
    }
    infoLabel_->setText(QString("%1 secrets | %2 rotated | %3 expired")
        .arg(entries_.size()).arg(rotatedCount()).arg(expiredCount()));
}

void PaperSecretVault::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SecretEntry e;
        e.id       = settings_.value("id").toInt();
        e.name     = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.type     = settings_.value("type").toString();
        e.created  = settings_.value("created").toString();
        e.lastUsed = settings_.value("lastUsed").toString();
        e.rotated  = settings_.value("rotated").toBool();
        e.expired  = settings_.value("expired").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSecretVault::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("name",     entries_[i].name);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("type",     entries_[i].type);
        settings_.setValue("created",  entries_[i].created);
        settings_.setValue("lastUsed", entries_[i].lastUsed);
        settings_.setValue("rotated",  entries_[i].rotated);
        settings_.setValue("expired",  entries_[i].expired);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
