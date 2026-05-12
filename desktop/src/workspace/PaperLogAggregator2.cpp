#include "workspace/PaperLogAggregator2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperLogAggregator2::PaperLogAggregator2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LogAggregator2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList sources = {"API Server", "Database", "Cache Layer", "Auth Service"};
        QStringList levels = {"INFO", "WARN", "ERROR", "FATAL"};
        QStringList categories = {"Application", "Infrastructure", "Security", "Network", "Database"};
        QColor palette[] = {
            QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
            QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
            QColor(0x7c, 0x3a, 0xed)
        };
        for (int i = 0; i < 8; ++i) {
            LogAggregator2Entry e;
            e.id = i + 1;
            e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
            e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
            e.volume = 10.0 + QRandomGenerator::global()->bounded(9000) / 100.0;
            e.alerts = QRandomGenerator::global()->bounded(20);
            e.critical = e.level == "FATAL" || (e.level == "ERROR" && e.alerts > 10);
            e.color = palette[QRandomGenerator::global()->bounded(5)];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperLogAggregator2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Application", "Infrastructure", "Security", "Network", "Database"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search sources...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    aggregateBtn_ = new QPushButton("Aggregate");
    aggregateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(aggregateBtn_, &QPushButton::clicked, this, &PaperLogAggregator2::onAggregate);
    toolbar->addWidget(aggregateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLogAggregator2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Log aggregator");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperLogAggregator2::addEntry(const LogAggregator2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit alertTriggered(entry.id, entry.volume);
    update();
}

QList<LogAggregator2Entry> PaperLogAggregator2::entries() const { return entries_; }

int PaperLogAggregator2::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) ++c;
    return c;
}

qreal PaperLogAggregator2::avgVolume() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.volume;
    return sum / entries_.size();
}

QMap<QString, int> PaperLogAggregator2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLogAggregator2::onAggregate() {
    QStringList sources = {"API Server", "Database", "Cache Layer", "Auth Service"};
    QStringList levels = {"INFO", "WARN", "ERROR", "FATAL"};
    QStringList categories = {"Application", "Infrastructure", "Security", "Network", "Database"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    LogAggregator2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
    int cIdx = categoryCombo_->currentIndex();
    e.category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
    e.volume = 10.0 + QRandomGenerator::global()->bounded(9000) / 100.0;
    e.alerts = QRandomGenerator::global()->bounded(20);
    e.critical = e.level == "FATAL" || (e.level == "ERROR" && e.alerts > 10);
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperLogAggregator2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Log aggregator");
    update();
}

void PaperLogAggregator2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Log aggregator");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Log Aggregator");

    int w = width();
    int h = height();
    int statsH = static_cast<int>(h * 0.25);
    int topH = h - statsH - 70;

    drawAggregatorView(p, QRect(10, 50, static_cast<int>(w * 0.6) - 10, topH));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6) + 10, 50,
                               static_cast<int>(w * 0.4) - 20, topH));
    drawStats(p, QRect(20, h - statsH, w - 40, statsH - 10));
}

void PaperLogAggregator2::drawAggregatorView(QPainter& p, const QRect& rect) {
    QString filter = inputField_->text().trimmed().toLower();
    int cIdx = categoryCombo_->currentIndex();
    QString catFilter = cIdx > 0 ? categoryCombo_->itemText(cIdx) : QString();

    QList<const LogAggregator2Entry*> visible;
    for (const auto& e : entries_) {
        if (!catFilter.isEmpty() && e.category != catFilter) continue;
        if (!filter.isEmpty() && !e.source.toLower().contains(filter)) continue;
        visible.append(&e);
    }

    int show = qMin(8, visible.size());
    if (show == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching log sources");
        return;
    }

    int itemH = qMin(52, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = *visible[i];
        int y = rect.y() + i * (itemH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 6, 6);

        // Color accent bar on left
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Source name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 12, y + 3, rect.width() / 2 - 12, 16,
                   Qt::AlignVCenter, e.source);

        // Level badge
        QColor levelColor;
        if (e.level == "INFO")       levelColor = QColor(0x3b, 0x82, 0xf6);
        else if (e.level == "WARN")  levelColor = QColor(0xd9, 0x77, 0x06);
        else if (e.level == "ERROR") levelColor = QColor(0xdc, 0x26, 0x26);
        else                         levelColor = QColor(0x7c, 0x3a, 0xed); // FATAL=purple

        p.setPen(Qt::NoPen);
        p.setBrush(levelColor);
        int badgeW = 50;
        int badgeX = rect.x() + rect.width() - badgeW - 8;
        p.drawRoundedRect(badgeX, y + 3, badgeW, 16, 8, 8);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, y + 3, badgeW, 16, Qt::AlignCenter, e.level);

        // Critical warning indicator
        int dotX = rect.x() + 12;
        int dotY = y + 22;
        if (e.critical) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0xdc, 0x26, 0x26));
            p.drawEllipse(dotX, dotY + 2, 8, 8);
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(dotX + 12, dotY, 50, 12, Qt::AlignVCenter, "CRITICAL");
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0x16, 0xa3, 0x4a));
            p.drawEllipse(dotX, dotY + 2, 8, 8);
            p.setPen(QColor(100, 116, 139));
            p.setFont(QFont("Arial", 7));
            p.drawText(dotX + 12, dotY, 50, 12, Qt::AlignVCenter, "Normal");
        }

        // Volume bar
        int barX = dotX + 80;
        int barW = rect.width() / 2 - 100;
        int barY = dotY + 3;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, 6, 3, 3);

        qreal maxVolume = 100.0;
        for (const auto& entry : entries_) maxVolume = qMax(maxVolume, entry.volume);
        qreal ratio = qMin(e.volume / maxVolume, 1.0);
        int fillW = static_cast<int>(ratio * barW);
        p.setBrush(levelColor);
        p.drawRoundedRect(barX, barY, fillW, 6, 3, 3);

        // Volume text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, dotY, 50, 12, Qt::AlignVCenter,
                   QString::number(e.volume, 'f', 1));

        // Alert count on the right
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, dotY,
                   rect.width() / 2 - 80, 12,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.alerts) + " alerts");
    }
}

void PaperLogAggregator2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Level Distribution");

    QStringList levels = {"INFO", "WARN", "ERROR", "FATAL"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26), QColor(0x7c, 0x3a, 0xed)
    };

    QMap<QString, int> levelCounts;
    for (const auto& e : entries_) levelCounts[e.level]++;

    int maxVal = 1;
    for (const auto& v : levelCounts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 40) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 24 + i * (barH + 4);
        int count = levelCounts.contains(levels[i]) ? levelCounts[levels[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 4, 50, barH, Qt::AlignRight | Qt::AlignVCenter,
                   levels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 56, y, barW, barH - 4, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 60 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperLogAggregator2::drawStats(QPainter& p, const QRect& rect) {
    int totalAlerts = 0;
    for (const auto& e : entries_) totalAlerts += e.alerts;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Sources",  QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Critical Count", QString::number(criticalCount()), QColor(0xdc, 0x26, 0x26)},
        {"Avg Volume",     QString::number(avgVolume(), 'f', 1),
         QColor(0xd9, 0x77, 0x06)},
        {"Total Alerts",   QString::number(totalAlerts),    QColor(0x7c, 0x3a, 0xed)}
    };

    int boxW = (rect.width() - 30) / 4;
    int boxH = qMin(56, rect.height() - 4);

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        // Top color accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 3, 2, 2);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, y + 8, boxW - 20, 24, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + 32, boxW - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperLogAggregator2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Log aggregator");
        return;
    }
    infoLabel_->setText(
        QString("%1 sources | %2 critical | avg vol %3")
            .arg(entries_.size())
            .arg(criticalCount())
            .arg(avgVolume(), 0, 'f', 1));
}

void PaperLogAggregator2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LogAggregator2Entry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.category = settings_.value("category").toString();
        e.level = settings_.value("level").toString();
        e.volume = settings_.value("volume").toDouble();
        e.alerts = settings_.value("alerts").toInt();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLogAggregator2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("level", entries_[i].level);
        settings_.setValue("volume", entries_[i].volume);
        settings_.setValue("alerts", entries_[i].alerts);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
