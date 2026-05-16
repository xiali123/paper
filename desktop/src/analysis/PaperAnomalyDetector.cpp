#include "analysis/PaperAnomalyDetector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperAnomalyDetector::PaperAnomalyDetector(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperAnomalyDetector::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Citation", "Author", "Publication", "Metric", "Content"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Metric name...");
    detectBtn_ = new QPushButton("Detect", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Anomalies: 0 | Critical: 0 | Max Deviation: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(detectBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(detectBtn_, &QPushButton::clicked, this, &PaperAnomalyDetector::onDetect);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAnomalyDetector::onClear);
}

void PaperAnomalyDetector::addEntry(const AnomalyEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<AnomalyEntry> PaperAnomalyDetector::entries() const { return entries_; }

int PaperAnomalyDetector::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.critical) c++;
    return c;
}

qreal PaperAnomalyDetector::maxDeviation() const {
    qreal mx = 0;
    for (const auto& e : entries_) mx = qMax(mx, qAbs(e.deviation));
    return mx;
}

QMap<QString, int> PaperAnomalyDetector::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperAnomalyDetector::onDetect() {
    AnomalyEntry e;
    e.id = entries_.size() + 1;
    e.metric = inputField_->text().trimmed();
    if (e.metric.isEmpty()) e.metric = QString("Metric_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList severities = {"Low", "Medium", "High", "Critical"};
    e.severity = severities[QRandomGenerator::global()->bounded(severities.size())];
    e.value = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.threshold = 50.0;
    e.deviation = e.value - e.threshold;
    e.critical = qAbs(e.deviation) > 30;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit anomalyDetected(e.id, e.deviation);
    update();
}

void PaperAnomalyDetector::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperAnomalyDetector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawAnomalyList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperAnomalyDetector::drawAnomalyList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Anomaly Detection Results:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        QColor dotColor = e.critical ? QColor(0xdc2626) : (e.severity == "High" ? QColor(0xd97706) : QColor(0x3b82f6));
        p.setPen(dotColor);
        p.setBrush(dotColor);
        p.drawEllipse(rect.left(), y + 1, 8, 8);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | Val: %3 | Dev: %4 | %5")
            .arg(e.metric, e.severity)
            .arg(QString::number(e.value, 'f', 1))
            .arg(QString::number(e.deviation, 'f', 1))
            .arg(e.critical ? "CRITICAL" : "Normal");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperAnomalyDetector::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Source:");
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

void PaperAnomalyDetector::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Critical: %1").arg(criticalCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Max Deviation: %1").arg(QString::number(maxDeviation(), 'f', 2)));
}

void PaperAnomalyDetector::updateInfo() {
    infoLabel_->setText(QString("Anomalies: %1 | Critical: %2 | Max Deviation: %3")
        .arg(entries_.size()).arg(criticalCount())
        .arg(QString::number(maxDeviation(), 'f', 2)));
}

void PaperAnomalyDetector::loadSettings() {
    settings_.beginGroup("AnomalyDetector");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        AnomalyEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.metric = settings_.value(QString("metric_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.severity = settings_.value(QString("severity_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.threshold = settings_.value(QString("threshold_%1").arg(i)).toDouble();
        e.deviation = settings_.value(QString("deviation_%1").arg(i)).toDouble();
        e.critical = settings_.value(QString("critical_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperAnomalyDetector::saveSettings() {
    settings_.beginGroup("AnomalyDetector");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("metric_%1").arg(i), e.metric);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("severity_%1").arg(i), e.severity);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("threshold_%1").arg(i), e.threshold);
        settings_.setValue(QString("deviation_%1").arg(i), e.deviation);
        settings_.setValue(QString("critical_%1").arg(i), e.critical);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
