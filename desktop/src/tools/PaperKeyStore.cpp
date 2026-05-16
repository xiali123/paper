#include "tools/PaperKeyStore.hpp"
#include <QDateTime>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QFontMetrics>
#include <algorithm>

namespace {
const QVector<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};
}

PaperKeyStore::PaperKeyStore(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "KeyStore")
    , storeBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperKeyStore::setupUI()
{
    auto* toolbar = new QHBoxLayout;

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter key name...");
    inputField_->setMinimumWidth(200);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"API Key", "Certificate", "Token", "Password", "SSH Key"});

    storeBtn_ = new QPushButton("Store", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    toolbar->addWidget(inputField_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(storeBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    toolbar->addWidget(infoLabel_);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(toolbar);
    mainLayout->addStretch(1);

    setLayout(mainLayout);
    setMinimumSize(720, 480);

    connect(storeBtn_, &QPushButton::clicked, this, &PaperKeyStore::onStore);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperKeyStore::onClear);

    updateInfo();
}

void PaperKeyStore::addEntry(const KeyEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<KeyEntry> PaperKeyStore::entries() const
{
    return entries_;
}

int PaperKeyStore::activeCount() const
{
    return std::count_if(entries_.cbegin(), entries_.cend(),
        [](const KeyEntry& e) { return e.active; });
}

int PaperKeyStore::expiredCount() const
{
    return std::count_if(entries_.cbegin(), entries_.cend(),
        [](const KeyEntry& e) { return e.expired; });
}

QMap<QString, int> PaperKeyStore::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperKeyStore::onStore()
{
    const QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    static const QStringList types = {"RSA-2048", "AES-256", "ECDSA-P256", "Ed25519", "HMAC-SHA256"};
    const QString type = types[QRandomGenerator::global()->bounded(types.size())];

    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime exp = now.addDays(30 + QRandomGenerator::global()->bounded(335));

    KeyEntry entry;
    entry.id = QRandomGenerator::global()->bounded(1, 999999);
    entry.name = name;
    entry.category = categoryCombo_->currentText();
    entry.type = type;
    entry.created = now.toString(Qt::ISODate);
    entry.expires = exp.toString(Qt::ISODate);
    entry.active = true;
    entry.expired = exp < now;
    entry.color = kPalette[QRandomGenerator::global()->bounded(kPalette.size())];

    addEntry(entry);
    emit keyStored(entry.id, entry.type);

    inputField_->clear();
}

void PaperKeyStore::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperKeyStore::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int toolbarH = 50;
    const QRect content(0, toolbarH, width(), height() - toolbarH);

    const int listW = static_cast<int>(content.width() * 0.5);
    const int rightW = content.width() - listW;
    const int chartH = static_cast<int>(content.height() * 0.55);

    drawKeyList(p, QRect(content.x(), content.y(), listW, content.height()));
    drawCategoryChart(p, QRect(content.x() + listW, content.y(), rightW, chartH));
    drawStats(p, QRect(content.x() + listW, content.y() + chartH, rightW, content.height() - chartH));
}

void PaperKeyStore::drawKeyList(QPainter& p, const QRect& rect)
{
    p.save();

    QPainterPath bg;
    bg.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bg, QColor(248, 250, 252));

    QPen borderPen(QColor(226, 232, 240));
    borderPen.setWidth(1);
    p.setPen(borderPen);
    p.drawPath(bg);

    int y = rect.y() + 18;
    QFont headerFont = font();
    headerFont.setBold(true);
    headerFont.setPointSize(11);
    p.setFont(headerFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.x() + 16, y, "Key Store");
    y += 28;

    QFont entryFont = font();
    entryFont.setPointSize(9);
    p.setFont(entryFont);

    if (entries_.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.x() + 16, y + 20, "No keys stored yet.");
        p.restore();
        return;
    }

    const int rowH = 48;
    const int maxVisible = (rect.height() - 50) / rowH;

    for (int i = 0; i < std::min(entries_.size(), maxVisible); ++i) {
        const auto& entry = entries_[i];
        const QRect row(rect.x() + 8, y, rect.width() - 16, rowH - 4);

        QPainterPath rowBg;
        rowBg.addRoundedRect(row, 6, 6);

        QColor rowColor = QColor(255, 255, 255);
        if (entry.expired) {
            rowColor = QColor(254, 242, 242);
        } else if (!entry.active) {
            rowColor = QColor(241, 245, 249);
        }
        p.fillPath(rowBg, rowColor);

        QPen rowPen(QColor(226, 232, 240));
        rowPen.setWidth(1);
        p.setPen(rowPen);
        p.drawPath(rowBg);

        // Color indicator
        p.setPen(Qt::NoPen);
        p.setBrush(entry.color);
        p.drawEllipse(row.x() + 10, row.y() + row.height() / 2 - 5, 10, 10);

        // Name and type
        p.setPen(QColor(30, 41, 59));
        QFont nameFont = entryFont;
        nameFont.setBold(true);
        p.setFont(nameFont);
        p.drawText(row.x() + 28, row.y() + 18, entry.name);

        p.setFont(entryFont);
        p.setPen(QColor(100, 116, 139));
        p.drawText(row.x() + 28, row.y() + 34,
                   QString("%1 | %2").arg(entry.type, entry.category));

        // Status badge
        if (entry.expired) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#dc2626"));
            const QRect badge(row.right() - 64, row.y() + 14, 56, 20);
            QPainterPath badgePath;
            badgePath.addRoundedRect(badge, 10, 10);
            p.drawPath(badgePath);
            p.setPen(Qt::white);
            p.setFont(entryFont);
            p.drawText(badge, Qt::AlignCenter, "Expired");
        } else if (entry.active) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#16a34a"));
            const QRect badge(row.right() - 52, row.y() + 14, 44, 20);
            QPainterPath badgePath;
            badgePath.addRoundedRect(badge, 10, 10);
            p.drawPath(badgePath);
            p.setPen(Qt::white);
            p.setFont(entryFont);
            p.drawText(badge, Qt::AlignCenter, "Active");
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#94a3b8"));
            const QRect badge(row.right() - 60, row.y() + 14, 52, 20);
            QPainterPath badgePath;
            badgePath.addRoundedRect(badge, 10, 10);
            p.drawPath(badgePath);
            p.setPen(Qt::white);
            p.setFont(entryFont);
            p.drawText(badge, Qt::AlignCenter, "Inactive");
        }

        y += rowH;
    }

    if (entries_.size() > maxVisible) {
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.x() + 16, y + 16,
                   QString("... and %1 more").arg(entries_.size() - maxVisible));
    }

    p.restore();
}

void PaperKeyStore::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.save();

    QPainterPath bg;
    bg.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bg, QColor(248, 250, 252));

    QPen borderPen(QColor(226, 232, 240));
    borderPen.setWidth(1);
    p.setPen(borderPen);
    p.drawPath(bg);

    QFont headerFont = font();
    headerFont.setBold(true);
    headerFont.setPointSize(11);
    p.setFont(headerFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.x() + 16, rect.y() + 22, "Categories");

    const auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont emptyFont = font();
        emptyFont.setPointSize(9);
        p.setFont(emptyFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.x() + 16, rect.y() + 50, "No data.");
        p.restore();
        return;
    }

    const int total = entries_.size();
    const int cx = rect.x() + rect.width() / 3;
    const int cy = rect.y() + rect.height() / 2 + 10;
    const int radius = std::min(rect.width() / 3, rect.height() / 2 - 40);

    int idx = 0;
    int startAngle = 0;
    QFont labelFont = font();
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    for (auto it = counts.cbegin(); it != counts.cend(); ++it, ++idx) {
        const int span = static_cast<int>(360.0 * it.value() / total * 16);
        const QColor color = kPalette[idx % kPalette.size()];

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPie(cx - radius, cy - radius, radius * 2, radius * 2, startAngle, span);

        const double midAngle = (startAngle / 16.0 + span / 32.0) * M_PI / 180.0;
        const int lx = cx + static_cast<int>((radius + 20) * std::cos(midAngle));
        const int ly = cy - static_cast<int>((radius + 20) * std::sin(midAngle));

        p.setPen(QColor(30, 41, 59));
        p.drawText(lx - 30, ly - 4, 60, 16, Qt::AlignCenter,
                   QString("%1 (%2)").arg(it.key()).arg(it.value()));

        startAngle += span;
    }

    p.restore();
}

void PaperKeyStore::drawStats(QPainter& p, const QRect& rect)
{
    p.save();

    QPainterPath bg;
    bg.addRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);
    p.fillPath(bg, QColor(248, 250, 252));

    QPen borderPen(QColor(226, 232, 240));
    borderPen.setWidth(1);
    p.setPen(borderPen);
    p.drawPath(bg);

    QFont headerFont = font();
    headerFont.setBold(true);
    headerFont.setPointSize(11);
    p.setFont(headerFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.x() + 16, rect.y() + 22, "Statistics");

    const int total = entries_.size();
    const int active = activeCount();
    const int expired = expiredCount();
    const int inactive = total - active - expired;

    struct Stat { QString label; int value; QColor color; };
    const Stat stats[] = {
        {"Total",    total,   QColor("#3b82f6")},
        {"Active",   active,  QColor("#16a34a")},
        {"Expired",  expired, QColor("#dc2626")},
        {"Inactive", inactive, QColor("#94a3b8")}
    };

    const int barY = rect.y() + 36;
    const int barH = 20;
    const int barW = rect.width() - 32;
    const int barX = rect.x() + 16;

    QFont statFont = font();
    statFont.setPointSize(9);
    p.setFont(statFont);

    int y = barY;
    for (const auto& s : stats) {
        p.setPen(QColor(30, 41, 59));
        p.drawText(barX, y + 12, QString("%1: %2").arg(s.label).arg(s.value));

        if (total > 0) {
            const int fillW = static_cast<int>(barW * static_cast<double>(s.value) / total);
            p.setPen(Qt::NoPen);
            p.setBrush(s.color);
            QPainterPath barPath;
            barPath.addRoundedRect(barX, y + 16, fillW, barH, 4, 4);
            p.drawPath(barPath);
        }
        y += barH + 28;
    }

    p.restore();
}

void PaperKeyStore::updateInfo()
{
    infoLabel_->setText(
        QString("Total: %1  |  Active: %2  |  Expired: %3")
            .arg(entries_.size())
            .arg(activeCount())
            .arg(expiredCount()));
}

void PaperKeyStore::loadSettings()
{
    const int size = settings_.beginReadArray("keys");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        KeyEntry e;
        e.id       = settings_.value("id").toInt();
        e.name     = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.type     = settings_.value("type").toString();
        e.created  = settings_.value("created").toString();
        e.expires  = settings_.value("expires").toString();
        e.active   = settings_.value("active").toBool();
        e.expired  = settings_.value("expired").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
    update();
}

void PaperKeyStore::saveSettings()
{
    settings_.beginWriteArray("keys", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id",       e.id);
        settings_.setValue("name",     e.name);
        settings_.setValue("category", e.category);
        settings_.setValue("type",     e.type);
        settings_.setValue("created",  e.created);
        settings_.setValue("expires",  e.expires);
        settings_.setValue("active",   e.active);
        settings_.setValue("expired",  e.expired);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
