#include "analysis/PaperTopicTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTopicTracker::PaperTopicTracker(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperTopicTracker::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "NLP", "Vision", "RL", "Theory", "Systems"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Topic name...");
    trackBtn_ = new QPushButton("Track", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Topics: 0 | Rising: 0 | Avg Growth: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(trackBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(trackBtn_, &QPushButton::clicked, this, &PaperTopicTracker::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicTracker::onClear);
}

void PaperTopicTracker::addEntry(const TopicEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<TopicEntry> PaperTopicTracker::entries() const { return entries_; }

int PaperTopicTracker::risingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.rising) c++;
    return c;
}

qreal PaperTopicTracker::avgGrowth() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.growth;
    return sum / entries_.size();
}

QMap<QString, int> PaperTopicTracker::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperTopicTracker::onTrack() {
    TopicEntry e;
    e.id = entries_.size() + 1;
    e.topic = inputField_->text().trimmed();
    if (e.topic.isEmpty()) e.topic = QString("Topic_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList trends = {"Rising", "Stable", "Declining", "Emerging"};
    e.trend = trends[QRandomGenerator::global()->bounded(trends.size())];
    e.frequency = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.growth = QRandomGenerator::global()->bounded(-0.5, 1.5);
    e.papers = QRandomGenerator::global()->bounded(10, 5000);
    e.rising = e.growth > 0.3;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit topicTracked(e.id, e.growth);
    update();
}

void PaperTopicTracker::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperTopicTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawTopicList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperTopicTracker::drawTopicList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Topic Trends:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | Freq: %3 | Growth: %4 | %5")
            .arg(e.topic, e.category)
            .arg(QString::number(e.frequency, 'f', 1))
            .arg(QString::number(e.growth, 'f', 2))
            .arg(e.rising ? "Rising" : "Stable");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperTopicTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Field:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperTopicTracker::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Rising: %1").arg(risingCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Growth: %1").arg(QString::number(avgGrowth(), 'f', 3)));
}

void PaperTopicTracker::updateInfo() {
    infoLabel_->setText(QString("Topics: %1 | Rising: %2 | Avg Growth: %3")
        .arg(entries_.size()).arg(risingCount())
        .arg(QString::number(avgGrowth(), 'f', 2)));
}

void PaperTopicTracker::loadSettings() {
    settings_.beginGroup("TopicTracker");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        TopicEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.topic = settings_.value(QString("topic_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.trend = settings_.value(QString("trend_%1").arg(i)).toString();
        e.frequency = settings_.value(QString("frequency_%1").arg(i)).toDouble();
        e.growth = settings_.value(QString("growth_%1").arg(i)).toDouble();
        e.papers = settings_.value(QString("papers_%1").arg(i)).toInt();
        e.rising = settings_.value(QString("rising_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperTopicTracker::saveSettings() {
    settings_.beginGroup("TopicTracker");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("topic_%1").arg(i), e.topic);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("trend_%1").arg(i), e.trend);
        settings_.setValue(QString("frequency_%1").arg(i), e.frequency);
        settings_.setValue(QString("growth_%1").arg(i), e.growth);
        settings_.setValue(QString("papers_%1").arg(i), e.papers);
        settings_.setValue(QString("rising_%1").arg(i), e.rising);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
