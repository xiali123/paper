#include "analysis/PaperTopicSentinel2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperTopicSentinel2::PaperTopicSentinel2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TopicSentinel2")
{
    setupUI();
    loadSettings();
}

void PaperTopicSentinel2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "AI", "Blockchain", "Quantum", "Biotech", "GreenTech"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter topic...");
    monitorBtn_ = new QPushButton("Monitor", this);
    monitorBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("color: #dc2626;");
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_, 1);
    toolbar->addWidget(monitorBtn_);
    toolbar->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Topics: 0 | Emerging: 0 | Avg Momentum: 0.00", this);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");

    layout->addLayout(toolbar);
    layout->addWidget(infoLabel_);
    setMinimumSize(640, 520);

    connect(monitorBtn_, &QPushButton::clicked, this, &PaperTopicSentinel2::onMonitor);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicSentinel2::onClear);
}

void PaperTopicSentinel2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));

    if (entries_.isEmpty()) {
        p.setPen(QColor(0xcbd5e1));
        p.setFont(QFont("Sans", 12));
        p.drawText(rect(), Qt::AlignCenter, "Monitor topics");
        return;
    }

    p.setPen(QColor(0x0f172a));
    p.setFont(QFont("Sans", 13, QFont::Bold));
    p.drawText(20, 30, "Topic Sentinel");

    int w = width(), h = height();
    drawSentinelView(p, QRect(20, 50, w - 40, h / 2 - 30));
    drawCategoryChart(p, QRect(20, h / 2 + 20, w / 2 - 20, h / 2 - 50));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTopicSentinel2::drawSentinelView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.topLeft(), "Sentinel Watch:");

    // Palette: green rising, amber stable, red declining, purple spike
    auto trendColor = [](const QString& trend) -> QColor {
        if (trend == "Rising")   return QColor(0x16a34a);
        if (trend == "Stable")   return QColor(0xd97706);
        if (trend == "Declining") return QColor(0xdc2626);
        if (trend == "Spike")    return QColor(0x7c3aed);
        return QColor(0x64748b);
    };

    int show = qMin(entries_.size(), 8);
    int cardH = qMin(52, (rect.height() - 20) / qMax(show, 1));
    int y = rect.y() + 20;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        QRect cardRect(rect.x(), y, rect.width(), cardH);
        QColor tc = trendColor(e.trend);

        // Card background
        QPainterPath card;
        card.addRoundedRect(cardRect, 8, 8);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(160));
        p.drawPath(card);

        // Left accent bar
        QPainterPath bar;
        bar.addRoundedRect(QRect(rect.x(), y, 6, cardH), 3, 3);
        p.setBrush(tc);
        p.drawPath(bar);

        // Topic name
        p.setPen(QColor(0x1e293b));
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(QRect(rect.x() + 14, y + 3, rect.width() / 2, 18),
                   Qt::AlignLeft | Qt::AlignVCenter, e.topic);

        // Trend badge
        p.setFont(QFont("Sans", 8));
        p.setPen(Qt::white);
        QRect badgeRect(rect.x() + 14, y + 24, 60, 16);
        QPainterPath badge;
        badge.addRoundedRect(badgeRect, 4, 4);
        p.setBrush(tc);
        p.drawPath(badge);
        p.drawText(badgeRect, Qt::AlignCenter, e.trend);

        // Emerging fire icon
        if (e.emerging) {
            p.setPen(QColor(0xdc2626));
            p.setFont(QFont("Sans", 10));
            p.drawText(QRect(rect.x() + 80, y + 22, 20, 18),
                       Qt::AlignCenter, QString::fromUtf8("\xe2\x98\x85"));
        }

        // Momentum bar
        int barX = rect.x() + rect.width() / 2 + 20;
        int barW = rect.width() / 4;
        int barY = y + 8;
        int barH = 10;

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xe2e8f0));
        p.drawRoundedRect(QRect(barX, barY, barW, barH), 4, 4);

        // Momentum fill (0.0-1.0)
        qreal clamped = qBound(0.0, e.momentum, 1.0);
        int fillW = static_cast<int>(clamped * barW);
        p.setBrush(tc);
        p.drawRoundedRect(QRect(barX, barY, fillW, barH), 4, 4);

        // Momentum label
        p.setPen(QColor(0x64748b));
        p.setFont(QFont("Sans", 7));
        p.drawText(QRect(barX, barY + barH, barW, 12),
                   Qt::AlignCenter,
                   QString("Momentum: %1%").arg(static_cast<int>(e.momentum * 100)));

        // Mention count
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        p.drawText(QRect(rect.x() + rect.width() - 100, y + 4, 90, 16),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString("Mentions: %1").arg(e.mentions));

        // Category label
        p.setPen(QColor(0x64748b));
        p.setFont(QFont("Sans", 7));
        p.drawText(QRect(rect.x() + rect.width() - 100, y + 22, 90, 16),
                   Qt::AlignRight | Qt::AlignVCenter, e.category);

        y += cardH + 6;
    }
}

void PaperTopicSentinel2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.topLeft(), "Category Distribution:");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor(0x94a3b8));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    int maxCount = 1;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxCount = qMax(maxCount, it.value());

    QMap<QString, QColor> catColors = {
        {"AI",        QColor(0x3b82f6)},
        {"Blockchain", QColor(0x16a34a)},
        {"Quantum",   QColor(0x7c3aed)},
        {"Biotech",   QColor(0xd97706)},
        {"GreenTech", QColor(0xdc2626)}
    };

    int barH = qMin(22, (rect.height() - 40) / qMax(counts.size(), 1));
    int y = rect.y() + 22;
    int barMaxW = rect.width() - 110;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QColor color = catColors.value(it.key(), QColor(0x64748b));

        // Label
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(rect.x(), y, 80, barH),
                   Qt::AlignRight | Qt::AlignVCenter, it.key());

        // Bar
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * barMaxW);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(QRect(rect.x() + 85, y + 2, barW, barH - 4), 4, 4);

        // Count
        p.setPen(QColor(0x334155));
        p.drawText(QRect(rect.x() + 90 + barW, y, 40, barH),
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += barH + 6;
    }
}

void PaperTopicSentinel2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Topics", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Emerging",     QString::number(emergingCount()), QColor(0x16a34a)},
        {"Avg Momentum", QString::number(avgMomentum() * 100, 'f', 0) + "%", QColor(0xd97706)},
        {"Categories",   QString::number(categoryCounts().size()), QColor(0x7c3aed)}
    };

    int boxW = (rect.width() - 15) / 2;
    int boxH = qMin(48, (rect.height() - 30) / 2);

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        int x = rect.x() + col * (boxW + 15);
        int y = rect.y() + 20 + row * (boxH + 10);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(QRect(x, y, boxW, boxH), 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Sans", 14, QFont::Bold));
        p.drawText(QRect(x + 10, y + 4, boxW - 20, 24),
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(0x64748b));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(x + 10, y + 28, boxW - 20, 16),
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTopicSentinel2::onMonitor() {
    static const QStringList trends = {"Rising", "Stable", "Declining", "Spike"};
    static const QStringList categories = {"AI", "Blockchain", "Quantum", "Biotech", "GreenTech"};
    static const QMap<QString, QColor> trendColors = {
        {"Rising",    QColor(0x16a34a)},
        {"Stable",    QColor(0xd97706)},
        {"Declining", QColor(0xdc2626)},
        {"Spike",     QColor(0x7c3aed)}
    };
    static const QMap<QString, QColor> catColors = {
        {"AI",        QColor(0x3b82f6)},
        {"Blockchain", QColor(0x16a34a)},
        {"Quantum",   QColor(0x7c3aed)},
        {"Biotech",   QColor(0xd97706)},
        {"GreenTech", QColor(0xdc2626)}
    };

    // Seed 8 entries if empty, otherwise add 1
    int count = entries_.isEmpty() ? 8 : 1;
    for (int i = 0; i < count; ++i) {
        TopicSentinel2Entry e;
        e.id = entries_.size() + 1;

        QString text = inputField_->text().trimmed();
        if (!text.isEmpty() && count == 1) {
            e.topic = text;
        } else {
            e.topic = QString("Topic_%1").arg(e.id);
        }

        QString selCat = categoryCombo_->currentText();
        if (selCat == "All") {
            selCat = categories[QRandomGenerator::global()->bounded(categories.size())];
        }
        e.category = selCat;
        e.trend = trends[QRandomGenerator::global()->bounded(trends.size())];
        e.momentum = QRandomGenerator::global()->bounded(100) / 100.0;
        e.mentions = QRandomGenerator::global()->bounded(5, 500);
        e.emerging = e.momentum > 0.7 && e.trend == "Spike";
        e.color = catColors.value(e.category, QColor(0x3b82f6));

        entries_.append(e);

        if (e.emerging && e.momentum > 0.8)
            emit topicSpiked(e.id, e.momentum);
    }

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperTopicSentinel2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperTopicSentinel2::addEntry(const TopicSentinel2Entry& entry) {
    entries_.append(entry);
    updateInfo();
    saveSettings();
    emit topicSpiked(entry.id, entry.momentum);
    update();
}

QList<TopicSentinel2Entry> PaperTopicSentinel2::entries() const {
    return entries_;
}

int PaperTopicSentinel2::emergingCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.emerging) ++c;
    return c;
}

qreal PaperTopicSentinel2::avgMomentum() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.momentum;
    return sum / entries_.size();
}

QMap<QString, int> PaperTopicSentinel2::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperTopicSentinel2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Monitor topics");
        return;
    }
    infoLabel_->setText(
        QString("Topics: %1 | Emerging: %2 | Avg Momentum: %3%")
            .arg(entries_.size())
            .arg(emergingCount())
            .arg(QString::number(avgMomentum() * 100, 'f', 0)));
}

void PaperTopicSentinel2::loadSettings() {
    int size = settings_.beginReadArray("sentinel2_entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TopicSentinel2Entry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.trend = settings_.value("trend").toString();
        e.momentum = settings_.value("momentum").toDouble();
        e.mentions = settings_.value("mentions").toInt();
        e.emerging = settings_.value("emerging").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTopicSentinel2::saveSettings() {
    settings_.beginWriteArray("sentinel2_entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("trend", entries_[i].trend);
        settings_.setValue("momentum", entries_[i].momentum);
        settings_.setValue("mentions", entries_[i].mentions);
        settings_.setValue("emerging", entries_[i].emerging);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
