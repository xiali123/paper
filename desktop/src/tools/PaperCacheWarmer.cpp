#include "tools/PaperCacheWarmer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCacheWarmer::PaperCacheWarmer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CacheWarmer")
{
    setupUI();
    loadSettings();
}

void PaperCacheWarmer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    warmBtn_ = new QPushButton("Warm");
    warmBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(warmBtn_, &QPushButton::clicked, this, &PaperCacheWarmer::onWarm);
    toolbar->addWidget(warmBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "API", "Database", "Compute", "Asset", "Session"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCacheWarmer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter cache key to warm...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Warm cache entries");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperCacheWarmer::addEntry(const CacheEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<CacheEntry> PaperCacheWarmer::entries() const { return entries_; }

int PaperCacheWarmer::warmCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.warm) c++;
    return c;
}

qreal PaperCacheWarmer::avgHitRate() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.hitRate;
    return sum / entries_.size();
}

QMap<QString, int> PaperCacheWarmer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCacheWarmer::onWarm() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"API", "Database", "Compute", "Asset", "Session"};
    QStringList tiers = {"L1", "L2", "L3", "CDN"};
    QStringList ttls = {"5m", "15m", "30m", "1h", "6h", "12h", "24h"};
    QColor palette[] = {QColor(59, 130, 246), QColor(22, 163, 106), QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)};
    int catIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        CacheEntry e;
        e.id = entries_.size() + 1;
        e.key = text.left(8).toLower() + ":cache" + QString::number(i);
        e.category = catIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[catIdx - 1];
        e.tier = tiers[QRandomGenerator::global()->bounded(tiers.size())];
        e.hitRate = QRandomGenerator::global()->generateDouble();
        e.size = 1 + QRandomGenerator::global()->bounded(2048);
        e.ttl = ttls[QRandomGenerator::global()->bounded(ttls.size())];
        e.warm = e.hitRate > 0.8;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
        if (e.warm) emit cacheWarmed(e.id, e.hitRate);
    }
    inputField_->clear();
}

void PaperCacheWarmer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Warm cache entries");
    update();
}

void PaperCacheWarmer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Warm cache entries");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Cache Warmer");
    int w = width(), h = height();
    drawCacheList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCacheWarmer::drawCacheList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.key.left(18) + (e.warm ? " [WARM]" : " [COLD]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.tier + " | " + QString::number(e.size) + "KB");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.hitRate * 100, 'f', 0) + "% hit");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "TTL:" + e.ttl);
    }
}

void PaperCacheWarmer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"API", "Database", "Compute", "Asset", "Session"};
    QColor colors[] = {QColor(59, 130, 246), QColor(22, 163, 106), QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCacheWarmer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Warmed", QString::number(warmCount()), QColor(22, 163, 106)},
        {"Avg Hit Rate", QString::number(avgHitRate() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
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

void PaperCacheWarmer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Warm cache entries"); return; }
    infoLabel_->setText(QString("%1 entries | %2 warmed | %3% hit")
        .arg(entries_.size()).arg(warmCount()).arg(avgHitRate() * 100, 0, 'f', 0));
}

void PaperCacheWarmer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CacheEntry e;
        e.id = settings_.value("id").toInt();
        e.key = settings_.value("key").toString();
        e.category = settings_.value("category").toString();
        e.tier = settings_.value("tier").toString();
        e.hitRate = settings_.value("hitRate").toDouble();
        e.size = settings_.value("size").toInt();
        e.ttl = settings_.value("ttl").toString();
        e.warm = settings_.value("warm").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCacheWarmer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("key", entries_[i].key);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("tier", entries_[i].tier);
        settings_.setValue("hitRate", entries_[i].hitRate);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("ttl", entries_[i].ttl);
        settings_.setValue("warm", entries_[i].warm);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
