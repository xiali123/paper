#include "reading/PaperReadingWaves.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingWaves::PaperReadingWaves(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperReadingWaves::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Alpha", "Beta", "Gamma", "Theta", "Delta"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    analyzeBtn_ = new QPushButton("Analyze", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Waves: 0 | Peaks: 0 | Total Energy: 0.00", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(analyzeBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperReadingWaves::onAnalyze);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingWaves::onClear);
}

void PaperReadingWaves::addEntry(const WaveEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<WaveEntry> PaperReadingWaves::entries() const { return entries_; }
int PaperReadingWaves::peakCount() const { int c = 0; for (const auto& e : entries_) if (e.peak) c++; return c; }
qreal PaperReadingWaves::totalEnergy() const { qreal s = 0; for (const auto& e : entries_) s += e.energy; return s; }
QMap<QString, int> PaperReadingWaves::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperReadingWaves::onAnalyze() {
    WaveEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList waves = {"Short", "Medium", "Long", "Sustained", "Burst"};
    e.wavelength = waves[QRandomGenerator::global()->bounded(waves.size())];
    e.amplitude = QRandomGenerator::global()->bounded(0.1, 10.0);
    e.frequency = QRandomGenerator::global()->bounded(0.5, 30.0);
    e.energy = e.amplitude * e.frequency;
    e.peak = e.amplitude > 7.0;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e); updateInfo(); saveSettings();
    emit waveAnalyzed(e.id, e.energy); update();
}

void PaperReadingWaves::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperReadingWaves::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));
    drawWaveView(p, QRect(10, 50, width() - 20, height() / 2 - 60));
    drawCategoryChart(p, QRect(10, height() / 2, width() / 2 - 10, height() / 2 - 60));
    drawStats(p, QRect(width() / 2 + 10, height() / 2, width() / 2 - 20, height() / 2 - 60));
}

void PaperReadingWaves::drawWaveView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reading Waves:");
    if (entries_.isEmpty()) return;
    int baseY = rect.top() + rect.height() / 2 + 10;
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::DashLine));
    p.drawLine(rect.left(), baseY, rect.right(), baseY);
    int stepW = qMax(6, (rect.width() - 20) / qMax(entries_.size() * 4, 1));
    int x = rect.left();
    for (int i = 0; i < qMin(entries_.size(), 10); ++i) {
        const auto& e = entries_[i];
        int amp = static_cast<int>(e.amplitude * 8);
        p.setPen(QPen(e.color, 2));
        p.drawLine(x, baseY, x + stepW, baseY - amp);
        p.drawLine(x + stepW, baseY - amp, x + stepW * 2, baseY + amp / 2);
        p.drawLine(x + stepW * 2, baseY + amp / 2, x + stepW * 3, baseY - amp / 3);
        p.drawLine(x + stepW * 3, baseY - amp / 3, x + stepW * 4, baseY);
        x += stepW * 4;
    }
}

void PaperReadingWaves::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Band:"); y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155)); p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value())); y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingWaves::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Peaks: %1").arg(peakCount())); y += 16;
    p.drawText(rect.left(), y, QString("Total Energy: %1").arg(QString::number(totalEnergy(), 'f', 1)));
}

void PaperReadingWaves::updateInfo() {
    infoLabel_->setText(QString("Waves: %1 | Peaks: %2 | Total Energy: %3")
        .arg(entries_.size()).arg(peakCount()).arg(QString::number(totalEnergy(), 'f', 1)));
}

void PaperReadingWaves::loadSettings() {
    settings_.beginGroup("ReadingWaves");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        WaveEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.wavelength = settings_.value(QString("wavelength_%1").arg(i)).toString();
        e.amplitude = settings_.value(QString("amplitude_%1").arg(i)).toDouble();
        e.frequency = settings_.value(QString("frequency_%1").arg(i)).toDouble();
        e.energy = settings_.value(QString("energy_%1").arg(i)).toDouble();
        e.peak = settings_.value(QString("peak_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperReadingWaves::saveSettings() {
    settings_.beginGroup("ReadingWaves"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("wavelength_%1").arg(i), e.wavelength);
        settings_.setValue(QString("amplitude_%1").arg(i), e.amplitude);
        settings_.setValue(QString("frequency_%1").arg(i), e.frequency);
        settings_.setValue(QString("energy_%1").arg(i), e.energy);
        settings_.setValue(QString("peak_%1").arg(i), e.peak);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
