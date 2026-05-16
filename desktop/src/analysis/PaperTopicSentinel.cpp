#include "analysis/PaperTopicSentinel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperTopicSentinel::PaperTopicSentinel(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "TopicSentinel") {
    setupUI();
    loadSettings();
}

void PaperTopicSentinel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Trending", "Emerging", "Declining", "Stable"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter topic...");
    monitorBtn_ = new QPushButton("Monitor", this);
    clearBtn_ = new QPushButton("Clear", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(monitorBtn_);
    toolbar->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Topics: 0 | Rising: 0 | Avg Trend: 0.00%", this);

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);

    connect(monitorBtn_, &QPushButton::clicked, this, &PaperTopicSentinel::onMonitor);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicSentinel::onClear);
}

void PaperTopicSentinel::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();

    p.fillRect(rect(), QColor(0xf8fafc));

    int topHeight = h / 2 - 30;
    int bottomHeight = h / 2 - 40;

    drawSentinelView(p, QRect(10, 50, w - 20, topHeight));
    drawCategoryChart(p, QRect(10, 50 + topHeight + 10, w / 2 - 10, bottomHeight));
    drawStats(p, QRect(w / 2 + 10, 50 + topHeight + 10, w / 2 - 20, bottomHeight));
}

void PaperTopicSentinel::drawSentinelView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Sentinel Watch:");

    int y = rect.top() + 24;
    int cardW = rect.width();
    int cardH = 52;

    for (int i = 0; i < qMin(entries_.size(), 10); ++i) {
        const auto& e = entries_[i];
        QRect cardRect(rect.left(), y, cardW, cardH);

        QPainterPath path;
        path.addRoundedRect(cardRect, 8, 8);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(160));
        p.drawPath(path);

        // Left color bar
        QPainterPath bar;
        bar.addRoundedRect(QRect(rect.left(), y, 6, cardH), 3, 3);
        p.setBrush(e.color);
        p.drawPath(bar);

        // Topic name
        p.setPen(QColor(0x1e293b));
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(QRect(rect.left() + 14, y + 4, cardW - 20, 18),
                   Qt::AlignLeft | Qt::AlignVCenter, e.topic);

        // Category badge
        p.setFont(QFont("Sans", 8));
        p.setPen(Qt::white);
        QRect badgeRect(rect.left() + 14, y + 24, 60, 16);
        QPainterPath badge;
        badge.addRoundedRect(badgeRect, 4, 4);
        p.setBrush(e.color);
        p.drawPath(badge);
        p.drawText(badgeRect, Qt::AlignCenter, e.category);

        // Trend percentage
        p.setPen(e.trend >= 0 ? QColor(0x16a34a) : QColor(0xdc2626));
        p.setFont(QFont("Sans", 9));
        QString trendStr = QString("%1%2%")
                               .arg(e.trend >= 0 ? "+" : "")
                               .arg(QString::number(e.trend, 'f', 1));
        p.drawText(QRect(rect.left() + 80, y + 24, 80, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, trendStr);

        // Alert level
        p.setPen(QColor(0x64748b));
        p.setFont(QFont("Sans", 8));
        p.drawText(QRect(rect.left() + 170, y + 24, 120, 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("Alert: %1").arg(e.alert));

        // Mentions count
        p.drawText(QRect(rect.left() + 300, y + 24, 100, 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("Mentions: %1").arg(e.mentions));

        // Rising indicator
        if (e.rising) {
            p.setPen(QColor(0x16a34a));
            p.setFont(QFont("Sans", 10, QFont::Bold));
            p.drawText(QRect(cardRect.right() - 70, y + 4, 60, 20),
                       Qt::AlignRight | Qt::AlignVCenter, "▲ Rising");
        } else {
            p.setPen(QColor(0x94a3b8));
            p.setFont(QFont("Sans", 9));
            p.drawText(QRect(cardRect.right() - 70, y + 4, 60, 20),
                       Qt::AlignRight | Qt::AlignVCenter, "— Flat");
        }

        y += cardH + 6;
    }

    p.setBrush(Qt::NoBrush);
}

void PaperTopicSentinel::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Category Distribution:");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor(0x94a3b8));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxCount = qMax(maxCount, it.value());

    QMap<QString, QColor> catColors = {
        {"Trending", QColor(0x3b82f6)},
        {"Emerging", QColor(0x16a34a)},
        {"Declining", QColor(0x7c3aed)},
        {"Stable", QColor(0xd97706)}
    };

    int y = rect.top() + 24;
    int barMaxW = rect.width() - 100;
    int barH = 18;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QColor color = catColors.value(it.key(), QColor(0x64748b));

        // Label
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(rect.left(), y, 80, barH),
                   Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar
        int barW = maxCount > 0 ? (it.value() * barMaxW / maxCount) : 0;
        QPainterPath bar;
        bar.addRoundedRect(QRect(rect.left() + 85, y + 2, barW, barH - 4), 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(bar);

        // Count
        p.setPen(QColor(0x334155));
        p.drawText(QRect(rect.left() + 90 + barW, y, 40, barH),
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += barH + 6;
    }

    p.setBrush(Qt::NoBrush);
}

void PaperTopicSentinel::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");

    int y = rect.top() + 24;

    // Total topics
    p.setFont(QFont("Sans", 9));
    p.setPen(QColor(0x334155));
    p.drawText(rect.left(), y, QString("Total Topics: %1").arg(entries_.size()));
    y += 20;

    // Rising count
    p.drawText(rect.left(), y, QString("Rising Topics: %1").arg(risingCount()));
    y += 20;

    // Avg trend
    p.drawText(rect.left(), y,
               QString("Avg Trend: %1%")
                   .arg(QString::number(avgTrend(), 'f', 2)));
    y += 20;

    // Category breakdown
    auto counts = categoryCounts();
    p.setFont(QFont("Sans", 9, QFont::Bold));
    p.drawText(rect.left(), y, "By Category:");
    y += 18;
    p.setFont(QFont("Sans", 9));
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.drawText(rect.left() + 10, y,
                   QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 16;
    }
}

void PaperTopicSentinel::onMonitor() {
    SentinelEntry e;
    e.id = entries_.size() + 1;
    e.topic = inputField_->text().trimmed();
    if (e.topic.isEmpty())
        e.topic = QString("Topic_%1").arg(e.id);

    QString selectedCategory = categoryCombo_->currentText();
    if (selectedCategory == "All") {
        QStringList cats = {"Trending", "Emerging", "Declining", "Stable"};
        selectedCategory = cats[QRandomGenerator::global()->bounded(cats.size())];
    }
    e.category = selectedCategory;

    // Alert levels based on category
    if (e.category == "Trending") {
        e.alert = "High";
        e.trend = QRandomGenerator::global()->bounded(10.0, 60.0);
        e.color = QColor(0x3b82f6);
    } else if (e.category == "Emerging") {
        e.alert = "Medium";
        e.trend = QRandomGenerator::global()->bounded(5.0, 30.0);
        e.color = QColor(0x16a34a);
    } else if (e.category == "Declining") {
        e.alert = "Warning";
        e.trend = QRandomGenerator::global()->bounded(-40.0, -5.0);
        e.color = QColor(0x7c3aed);
    } else { // Stable
        e.alert = "Low";
        e.trend = QRandomGenerator::global()->bounded(-3.0, 5.0);
        e.color = QColor(0xd97706);
    }

    e.mentions = QRandomGenerator::global()->bounded(5, 500);
    e.rising = e.trend > 5.0;

    entries_.append(e);
    updateInfo();
    saveSettings();
    emit topicAlerted(e.id, e.trend);
    update();
}

void PaperTopicSentinel::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperTopicSentinel::updateInfo() {
    infoLabel_->setText(
        QString("Topics: %1 | Rising: %2 | Avg Trend: %3%")
            .arg(entries_.size())
            .arg(risingCount())
            .arg(QString::number(avgTrend(), 'f', 2)));
}

void PaperTopicSentinel::loadSettings() {
    settings_.beginGroup("TopicSentinel");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        SentinelEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.topic = settings_.value(QString("topic_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.alert = settings_.value(QString("alert_%1").arg(i)).toString();
        e.trend = settings_.value(QString("trend_%1").arg(i)).toDouble();
        e.mentions = settings_.value(QString("mentions_%1").arg(i)).toInt();
        e.rising = settings_.value(QString("rising_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperTopicSentinel::saveSettings() {
    settings_.beginGroup("TopicSentinel");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("topic_%1").arg(i), e.topic);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("alert_%1").arg(i), e.alert);
        settings_.setValue(QString("trend_%1").arg(i), e.trend);
        settings_.setValue(QString("mentions_%1").arg(i), e.mentions);
        settings_.setValue(QString("rising_%1").arg(i), e.rising);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}

void PaperTopicSentinel::addEntry(const SentinelEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<SentinelEntry> PaperTopicSentinel::entries() const {
    return entries_;
}

int PaperTopicSentinel::risingCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.rising) ++c;
    return c;
}

qreal PaperTopicSentinel::avgTrend() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.trend;
    return sum / entries_.size();
}

QMap<QString, int> PaperTopicSentinel::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}
