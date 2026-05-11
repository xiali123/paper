#include "visualization/PaperHistogramWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperHistogramWidget::PaperHistogramWidget(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperHistogramWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Citations", "Year", "Impact", "Relevance", "Pages"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Bin label...");
    generateBtn_ = new QPushButton("Generate", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Bins: 0 | Peaks: 0 | Max Count: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(generateBtn_, &QPushButton::clicked, this, &PaperHistogramWidget::onGenerate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHistogramWidget::onClear);
}

void PaperHistogramWidget::addEntry(const HistBin& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<HistBin> PaperHistogramWidget::entries() const { return entries_; }

int PaperHistogramWidget::peakCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.peak) c++;
    return c;
}

qreal PaperHistogramWidget::maxCount() const {
    qreal mx = 0;
    for (const auto& e : entries_) mx = qMax(mx, e.count);
    return mx;
}

QMap<QString, int> PaperHistogramWidget::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperHistogramWidget::onGenerate() {
    HistBin e;
    e.id = entries_.size() + 1;
    e.label = inputField_->text().trimmed();
    if (e.label.isEmpty()) e.label = QString("Bin_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.minVal = entries_.size() * 10.0;
    e.maxVal = e.minVal + 10.0;
    e.count = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.density = e.count / 10.0;
    e.peak = e.count > 70;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit histGenerated(e.id, e.count);
    update();
}

void PaperHistogramWidget::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperHistogramWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawHistView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryLegend(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperHistogramWidget::drawHistView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Histogram View:");
    if (entries_.isEmpty()) return;
    qreal mx = maxCount();
    if (mx <= 0) mx = 1;
    int barW = qMax(10, (rect.width() - 20) / qMax(entries_.size(), 1));
    int x = rect.left();
    int baseY = rect.bottom() - 10;
    for (int i = 0; i < qMin(entries_.size(), 20); ++i) {
        const auto& e = entries_[i];
        int barH = static_cast<int>((e.count / mx) * (rect.height() - 40));
        p.setBrush(e.color);
        p.setPen(Qt::NoPen);
        p.drawRect(x, baseY - barH, barW - 2, barH);
        p.setPen(QColor(0x334155));
        p.drawText(x, baseY + 12, e.label.left(6));
        x += barW;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperHistogramWidget::drawCategoryLegend(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
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

void PaperHistogramWidget::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total Bins: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Peaks: %1").arg(peakCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Max Count: %1").arg(QString::number(maxCount(), 'f', 2)));
}

void PaperHistogramWidget::updateInfo() {
    infoLabel_->setText(QString("Bins: %1 | Peaks: %2 | Max Count: %3")
        .arg(entries_.size()).arg(peakCount())
        .arg(QString::number(maxCount(), 'f', 2)));
}

void PaperHistogramWidget::loadSettings() {
    settings_.beginGroup("HistogramWidget");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        HistBin e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.minVal = settings_.value(QString("minVal_%1").arg(i)).toDouble();
        e.maxVal = settings_.value(QString("maxVal_%1").arg(i)).toDouble();
        e.count = settings_.value(QString("count_%1").arg(i)).toDouble();
        e.density = settings_.value(QString("density_%1").arg(i)).toDouble();
        e.peak = settings_.value(QString("peak_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperHistogramWidget::saveSettings() {
    settings_.beginGroup("HistogramWidget");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("minVal_%1").arg(i), e.minVal);
        settings_.setValue(QString("maxVal_%1").arg(i), e.maxVal);
        settings_.setValue(QString("count_%1").arg(i), e.count);
        settings_.setValue(QString("density_%1").arg(i), e.density);
        settings_.setValue(QString("peak_%1").arg(i), e.peak);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
