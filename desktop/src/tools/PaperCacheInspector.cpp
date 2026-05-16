#include "tools/PaperCacheInspector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCacheInspector::PaperCacheInspector(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CacheInspector")
{
    setupUI();
    loadSettings();
}

void PaperCacheInspector::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    inspectBtn_ = new QPushButton("Inspect");
    inspectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(inspectBtn_, &QPushButton::clicked, this, &PaperCacheInspector::onInspect);
    toolbar->addWidget(inspectBtn_);
    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"All", "Memory", "Disk", "Redis", "CDN"});
    toolbar->addWidget(typeCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCacheInspector::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter cache key pattern...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Inspect cache entries");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperCacheInspector::addEntry(const CacheEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit cacheInspected(entry.id, entry.hitRate);
    update();
}

QList<CacheEntry> PaperCacheInspector::entries() const { return entries_; }

qreal PaperCacheInspector::avgHitRate() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.hitRate;
    return sum / entries_.size();
}

int PaperCacheInspector::validCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.valid) c++;
    return c;
}

QMap<QString, int> PaperCacheInspector::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.cacheType]++;
    return counts;
}

void PaperCacheInspector::onInspect() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList types = {"memory", "disk", "redis", "cdn"};
    QStringList regions = {"us-east", "eu-west", "asia", "local"};
    QStringList ttls = {"5m", "15m", "1h", "6h", "24h", "7d"};
    int tIdx = typeCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        CacheEntry e;
        e.id = entries_.size() + 1;
        e.key = text.left(8).toLower() + ":entry" + QString::number(i);
        e.cacheType = tIdx == 0 ? types[QRandomGenerator::global()->bounded(types.size())] : types[tIdx - 1];
        e.sizeKB = 1 + QRandomGenerator::global()->bounded(1024);
        e.hits = QRandomGenerator::global()->bounded(1000);
        e.misses = QRandomGenerator::global()->bounded(200);
        e.hitRate = static_cast<qreal>(e.hits) / qMax(e.hits + e.misses, 1);
        e.ttl = ttls[QRandomGenerator::global()->bounded(ttls.size())];
        e.region = regions[QRandomGenerator::global()->bounded(regions.size())];
        e.valid = e.hitRate > 0.5;
        e.color = e.valid ? QColor(16,185,129) : (e.hitRate > 0.3 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCacheInspector::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Inspect cache entries");
    update();
}

void PaperCacheInspector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Inspect cache entries");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Cache Inspector");
    int w = width(), h = height();
    drawCacheList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCacheInspector::drawCacheList(QPainter& p, const QRect& rect) {
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
                   e.key.left(18) + (e.valid ? "" : " [STALE]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.cacheType + " | " + QString::number(e.sizeKB) + "KB | " + e.region);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.hitRate * 100, 'f', 0) + "% hit");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.hits) + "/" + QString::number(e.hits + e.misses) + " | TTL:" + e.ttl);
    }
}

void PaperCacheInspector::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Cache Types");
    auto counts = typeCounts();
    QStringList types = {"memory", "disk", "redis", "cdn"};
    QString labels[] = {"Memory", "Disk", "Redis", "CDN"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
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

void PaperCacheInspector::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Valid", QString::number(validCount()), QColor(16,185,129)},
        {"Avg Hit Rate", QString::number(avgHitRate() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperCacheInspector::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Inspect cache entries"); return; }
    infoLabel_->setText(QString("%1 entries | %2 valid | %3% hit")
        .arg(entries_.size()).arg(validCount()).arg(avgHitRate() * 100, 0, 'f', 0));
}

void PaperCacheInspector::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CacheEntry e;
        e.id = settings_.value("id").toInt();
        e.key = settings_.value("key").toString();
        e.cacheType = settings_.value("cacheType").toString();
        e.sizeKB = settings_.value("sizeKB").toInt();
        e.hits = settings_.value("hits").toInt();
        e.misses = settings_.value("misses").toInt();
        e.hitRate = settings_.value("hitRate").toDouble();
        e.ttl = settings_.value("ttl").toString();
        e.region = settings_.value("region").toString();
        e.valid = settings_.value("valid").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCacheInspector::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("key", entries_[i].key);
        settings_.setValue("cacheType", entries_[i].cacheType);
        settings_.setValue("sizeKB", entries_[i].sizeKB);
        settings_.setValue("hits", entries_[i].hits);
        settings_.setValue("misses", entries_[i].misses);
        settings_.setValue("hitRate", entries_[i].hitRate);
        settings_.setValue("ttl", entries_[i].ttl);
        settings_.setValue("region", entries_[i].region);
        settings_.setValue("valid", entries_[i].valid);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
