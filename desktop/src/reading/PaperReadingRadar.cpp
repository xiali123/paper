#include "reading/PaperReadingRadar.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingRadar::PaperReadingRadar(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperReadingRadar::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Clarity", "Rigor", "Novelty", "Impact", "Relevance"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    scanBtn_ = new QPushButton("Scan", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Scans: 0 | Top Quartile: 0 | Avg Score: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(scanBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(scanBtn_, &QPushButton::clicked, this, &PaperReadingRadar::onScan);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingRadar::onClear);
}

void PaperReadingRadar::addEntry(const RadarEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<RadarEntry> PaperReadingRadar::entries() const { return entries_; }

int PaperReadingRadar::topQuartileCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.topQuartile) c++;
    return c;
}

qreal PaperReadingRadar::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingRadar::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingRadar::onScan() {
    RadarEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.dimension = categoryCombo_->currentText();
    e.score = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.weight = QRandomGenerator::global()->bounded(0.5, 1.5);
    e.normalized = e.score * e.weight;
    e.topQuartile = e.normalized > 1.0;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit radarScanned(e.id, e.score);
    update();
}

void PaperReadingRadar::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingRadar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawRadarView(p, QRect(10, 50, w / 2, h - 60));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 20, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingRadar::drawRadarView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Radar Scan:");
    if (entries_.isEmpty()) return;
    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;
    // Draw grid circles
    p.setPen(QPen(QColor(0xe2e8f0), 1));
    for (int r = 1; r <= 4; ++r) {
        p.drawEllipse(cx - radius * r / 4, cy - radius * r / 4, radius * r / 2, radius * r / 2);
    }
    // Draw radar polygon
    int n = qMin(entries_.size(), 8);
    QPolygonF poly;
    for (int i = 0; i < n; ++i) {
        qreal angle = 2 * M_PI * i / n - M_PI / 2;
        qreal r = entries_[i].normalized * radius;
        poly << QPointF(cx + r * qCos(angle), cy + r * qSin(angle));
    }
    p.setPen(QPen(QColor(0x3b82f6), 2));
    p.setBrush(QColor(0x3b82f6, 40));
    p.drawPolygon(poly);
    // Draw axis labels
    p.setPen(QColor(0x334155));
    for (int i = 0; i < n; ++i) {
        qreal angle = 2 * M_PI * i / n - M_PI / 2;
        int lx = cx + (radius + 15) * qCos(angle);
        int ly = cy + (radius + 15) * qSin(angle);
        p.drawText(lx - 30, ly, entries_[i].dimension.left(8));
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingRadar::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Dimension:");
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

void PaperReadingRadar::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Top Quartile: %1").arg(topQuartileCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Score: %1").arg(QString::number(avgScore(), 'f', 3)));
}

void PaperReadingRadar::updateInfo() {
    infoLabel_->setText(QString("Scans: %1 | Top Quartile: %2 | Avg Score: %3")
        .arg(entries_.size()).arg(topQuartileCount())
        .arg(QString::number(avgScore(), 'f', 2)));
}

void PaperReadingRadar::loadSettings() {
    settings_.beginGroup("ReadingRadar");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        RadarEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.dimension = settings_.value(QString("dimension_%1").arg(i)).toString();
        e.score = settings_.value(QString("score_%1").arg(i)).toDouble();
        e.weight = settings_.value(QString("weight_%1").arg(i)).toDouble();
        e.normalized = settings_.value(QString("normalized_%1").arg(i)).toDouble();
        e.topQuartile = settings_.value(QString("topQuartile_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingRadar::saveSettings() {
    settings_.beginGroup("ReadingRadar");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("dimension_%1").arg(i), e.dimension);
        settings_.setValue(QString("score_%1").arg(i), e.score);
        settings_.setValue(QString("weight_%1").arg(i), e.weight);
        settings_.setValue(QString("normalized_%1").arg(i), e.normalized);
        settings_.setValue(QString("topQuartile_%1").arg(i), e.topQuartile);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
