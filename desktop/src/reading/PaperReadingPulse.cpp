#include "reading/PaperReadingPulse.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingPulse::PaperReadingPulse(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperReadingPulse::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Morning", "Afternoon", "Evening", "Night", "Weekend"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    measureBtn_ = new QPushButton("Measure", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Readings: 0 | Steady: 0 | Avg Focus: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(measureBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingPulse::onMeasure);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingPulse::onClear);
}

void PaperReadingPulse::addEntry(const PulseEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<PulseEntry> PaperReadingPulse::entries() const { return entries_; }

int PaperReadingPulse::steadyCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.steady) c++;
    return c;
}

qreal PaperReadingPulse::avgFocus() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.focus;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingPulse::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingPulse::onMeasure() {
    PulseEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList rhythms = {"Steady", "Accelerating", "Decelerating", "Spiky", "Flat"};
    e.rhythm = rhythms[QRandomGenerator::global()->bounded(rhythms.size())];
    e.speed = QRandomGenerator::global()->bounded(50.0, 300.0);
    e.focus = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.retention = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.steady = e.rhythm == "Steady" || e.focus > 0.7;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit pulseMeasured(e.id, e.focus);
    update();
}

void PaperReadingPulse::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingPulse::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawPulseView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingPulse::drawPulseView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reading Pulse:");
    if (entries_.isEmpty()) return;
    int x = rect.left();
    int baseY = rect.top() + rect.height() / 2 + 10;
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::DashLine));
    p.drawLine(rect.left(), baseY, rect.right(), baseY);
    p.setPen(QPen(QColor(0x3b82f6), 2));
    int stepW = qMax(5, (rect.width() - 20) / qMax(entries_.size() * 3, 1));
    for (int i = 0; i < qMin(entries_.size(), 10); ++i) {
        const auto& e = entries_[i];
        int h1 = static_cast<int>(e.focus * 40);
        int h2 = static_cast<int>(e.retention * 40);
        // Draw pulse wave
        p.drawLine(x, baseY, x + stepW, baseY - h1);
        p.drawLine(x + stepW, baseY - h1, x + stepW * 2, baseY + h2);
        p.drawLine(x + stepW * 2, baseY + h2, x + stepW * 3, baseY);
        x += stepW * 3;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingPulse::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Time:");
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

void PaperReadingPulse::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Steady: %1").arg(steadyCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Focus: %1").arg(QString::number(avgFocus(), 'f', 3)));
}

void PaperReadingPulse::updateInfo() {
    infoLabel_->setText(QString("Readings: %1 | Steady: %2 | Avg Focus: %3")
        .arg(entries_.size()).arg(steadyCount())
        .arg(QString::number(avgFocus(), 'f', 2)));
}

void PaperReadingPulse::loadSettings() {
    settings_.beginGroup("ReadingPulse");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        PulseEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.rhythm = settings_.value(QString("rhythm_%1").arg(i)).toString();
        e.speed = settings_.value(QString("speed_%1").arg(i)).toDouble();
        e.focus = settings_.value(QString("focus_%1").arg(i)).toDouble();
        e.retention = settings_.value(QString("retention_%1").arg(i)).toDouble();
        e.steady = settings_.value(QString("steady_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingPulse::saveSettings() {
    settings_.beginGroup("ReadingPulse");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("rhythm_%1").arg(i), e.rhythm);
        settings_.setValue(QString("speed_%1").arg(i), e.speed);
        settings_.setValue(QString("focus_%1").arg(i), e.focus);
        settings_.setValue(QString("retention_%1").arg(i), e.retention);
        settings_.setValue(QString("steady_%1").arg(i), e.steady);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
