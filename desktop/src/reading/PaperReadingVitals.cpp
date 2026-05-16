#include "reading/PaperReadingVitals.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingVitals::PaperReadingVitals(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperReadingVitals::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Comprehension", "Speed", "Retention", "Engagement", "Coverage"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    measureBtn_ = new QPushButton("Measure", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Vitals: 0 | Healthy: 0 | Avg Delta: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(measureBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingVitals::onMeasure);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingVitals::onClear);
}

void PaperReadingVitals::addEntry(const VitalEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<VitalEntry> PaperReadingVitals::entries() const { return entries_; }

int PaperReadingVitals::healthyCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.healthy) c++;
    return c;
}

qreal PaperReadingVitals::avgDelta() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.delta;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingVitals::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingVitals::onMeasure() {
    VitalEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.metric = categoryCombo_->currentText();
    e.value = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.baseline = 60.0;
    e.delta = e.value - e.baseline;
    e.healthy = e.delta > 0;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit vitalMeasured(e.id, e.value);
    update();
}

void PaperReadingVitals::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingVitals::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawVitalList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingVitals::drawVitalList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reading Vitals:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        // Vital bar
        int barW = static_cast<int>(e.value / 100.0 * (rect.width() - 200));
        QColor barColor = e.healthy ? QColor(0x16a34a) : QColor(0xdc2626);
        p.setBrush(QColor(0xe2e8f0));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y + 2, rect.width() - 200, 10, 2, 2);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.left(), y + 2, barW, 10, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + rect.width() - 195, y + 11, QString("%1 | %2 | %3%4")
            .arg(e.paper.left(10), e.metric)
            .arg(e.delta > 0 ? "+" : "")
            .arg(QString::number(e.delta, 'f', 1)));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingVitals::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Metric:");
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

void PaperReadingVitals::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Healthy: %1").arg(healthyCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Delta: %1").arg(QString::number(avgDelta(), 'f', 2)));
}

void PaperReadingVitals::updateInfo() {
    infoLabel_->setText(QString("Vitals: %1 | Healthy: %2 | Avg Delta: %3")
        .arg(entries_.size()).arg(healthyCount())
        .arg(QString::number(avgDelta(), 'f', 2)));
}

void PaperReadingVitals::loadSettings() {
    settings_.beginGroup("ReadingVitals");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        VitalEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.metric = settings_.value(QString("metric_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.baseline = settings_.value(QString("baseline_%1").arg(i)).toDouble();
        e.delta = settings_.value(QString("delta_%1").arg(i)).toDouble();
        e.healthy = settings_.value(QString("healthy_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingVitals::saveSettings() {
    settings_.beginGroup("ReadingVitals");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("metric_%1").arg(i), e.metric);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("baseline_%1").arg(i), e.baseline);
        settings_.setValue(QString("delta_%1").arg(i), e.delta);
        settings_.setValue(QString("healthy_%1").arg(i), e.healthy);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
