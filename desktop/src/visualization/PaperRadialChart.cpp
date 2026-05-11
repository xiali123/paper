#include "visualization/PaperRadialChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperRadialChart::PaperRadialChart(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperRadialChart::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Metrics", "Categories", "Timeline", "Network", "Quality"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Segment label...");
    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Segments: 0 | Highlighted: 0 | Max Value: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(renderBtn_, &QPushButton::clicked, this, &PaperRadialChart::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRadialChart::onClear);
}

void PaperRadialChart::addEntry(const RadialEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<RadialEntry> PaperRadialChart::entries() const { return entries_; }

int PaperRadialChart::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlighted) c++;
    return c;
}

qreal PaperRadialChart::maxValue() const {
    qreal mx = 0;
    for (const auto& e : entries_) mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperRadialChart::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperRadialChart::onRender() {
    RadialEntry e;
    e.id = entries_.size() + 1;
    e.label = inputField_->text().trimmed();
    if (e.label.isEmpty()) e.label = QString("Segment_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.ring = QString("Ring_%1").arg(QRandomGenerator::global()->bounded(1, 4));
    e.value = QRandomGenerator::global()->bounded(1.0, 100.0);
    e.angle = 360.0 / qMax(entries_.size() + 1, 1);
    e.radius = QRandomGenerator::global()->bounded(50.0, 150.0);
    e.highlighted = e.value > 70;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit radialRendered(e.id, e.value);
    update();
}

void PaperRadialChart::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperRadialChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawRadialView(p, QRect(10, 50, w / 2, h - 60));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 20, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperRadialChart::drawRadialView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Radial Chart:");
    if (entries_.isEmpty()) return;
    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int maxR = qMin(rect.width(), rect.height()) / 2 - 30;
    qreal mx = maxValue();
    if (mx <= 0) mx = 1;
    int n = entries_.size();
    qreal startAngle = 0;
    for (int i = 0; i < qMin(n, 12); ++i) {
        const auto& e = entries_[i];
        qreal spanAngle = 360.0 / n * 16;
        int r = static_cast<int>(e.value / mx * maxR);
        QRect arcRect(cx - r, cy - r, r * 2, r * 2);
        QColor c = e.color;
        c.setAlpha(e.highlighted ? 200 : 120);
        p.setBrush(c);
        p.setPen(QColor(0xffffff));
        p.drawPie(arcRect, static_cast<int>(startAngle), static_cast<int>(spanAngle));
        startAngle += spanAngle;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperRadialChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Group:");
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

void PaperRadialChart::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Segments: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Highlighted: %1").arg(highlightedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Max Value: %1").arg(QString::number(maxValue(), 'f', 1)));
}

void PaperRadialChart::updateInfo() {
    infoLabel_->setText(QString("Segments: %1 | Highlighted: %2 | Max Value: %3")
        .arg(entries_.size()).arg(highlightedCount())
        .arg(QString::number(maxValue(), 'f', 1)));
}

void PaperRadialChart::loadSettings() {
    settings_.beginGroup("RadialChart");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        RadialEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.ring = settings_.value(QString("ring_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.angle = settings_.value(QString("angle_%1").arg(i)).toDouble();
        e.radius = settings_.value(QString("radius_%1").arg(i)).toDouble();
        e.highlighted = settings_.value(QString("highlighted_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperRadialChart::saveSettings() {
    settings_.beginGroup("RadialChart");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("ring_%1").arg(i), e.ring);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("angle_%1").arg(i), e.angle);
        settings_.setValue(QString("radius_%1").arg(i), e.radius);
        settings_.setValue(QString("highlighted_%1").arg(i), e.highlighted);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
