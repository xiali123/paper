#include "visualization/PaperStepChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperStepChart::PaperStepChart(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperStepChart::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Accuracy", "Loss", "F1", "Speed", "Memory"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Step label...");
    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Steps: 0 | Improved: 0 | Max Delta: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(renderBtn_, &QPushButton::clicked, this, &PaperStepChart::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStepChart::onClear);
}

void PaperStepChart::addEntry(const StepEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<StepEntry> PaperStepChart::entries() const { return entries_; }

int PaperStepChart::improvedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.improved) c++;
    return c;
}

qreal PaperStepChart::maxDelta() const {
    qreal mx = 0;
    for (const auto& e : entries_) mx = qMax(mx, qAbs(e.delta));
    return mx;
}

QMap<QString, int> PaperStepChart::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperStepChart::onRender() {
    StepEntry e;
    e.id = entries_.size() + 1;
    e.step = inputField_->text().trimmed();
    if (e.step.isEmpty()) e.step = QString("Step_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList phases = {"Baseline", "Epoch 1", "Epoch 5", "Epoch 10", "Final"};
    e.phase = phases[QRandomGenerator::global()->bounded(phases.size())];
    e.value = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.baseline = entries_.isEmpty() ? e.value : entries_.first().value;
    e.delta = e.value - e.baseline;
    e.improved = e.delta > 0;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit stepRendered(e.id, e.delta);
    update();
}

void PaperStepChart::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperStepChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawStepView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryLegend(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperStepChart::drawStepView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Step Chart:");
    if (entries_.isEmpty()) return;
    int stepW = qMax(10, (rect.width() - 20) / qMax(entries_.size(), 1));
    int baseY = rect.top() + rect.height() / 2 + 10;
    // baseline
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::DashLine));
    p.drawLine(rect.left(), baseY, rect.right(), baseY);
    p.setPen(QColor(0x334155));
    int x = rect.left();
    qreal prevVal = 0;
    for (int i = 0; i < qMin(entries_.size(), 20); ++i) {
        const auto& e = entries_[i];
        int y = baseY - static_cast<int>(e.value / 100.0 * (rect.height() / 2 - 30));
        p.setBrush(e.color);
        p.setPen(Qt::NoPen);
        // Step line
        if (i > 0) {
            p.setPen(QPen(e.color, 2));
            int prevY = baseY - static_cast<int>(prevVal / 100.0 * (rect.height() / 2 - 30));
            p.drawLine(x - stepW, prevY, x, prevY);
            p.drawLine(x, prevY, x, y);
        }
        p.setBrush(e.color);
        p.drawEllipse(x - 3, y - 3, 6, 6);
        p.setPen(QColor(0x334155));
        p.drawText(x - 10, baseY + 14, e.step.left(8));
        prevVal = e.value;
        x += stepW;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperStepChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
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

void PaperStepChart::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total Steps: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Improved: %1").arg(improvedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Max Delta: %1").arg(QString::number(maxDelta(), 'f', 2)));
}

void PaperStepChart::updateInfo() {
    infoLabel_->setText(QString("Steps: %1 | Improved: %2 | Max Delta: %3")
        .arg(entries_.size()).arg(improvedCount())
        .arg(QString::number(maxDelta(), 'f', 2)));
}

void PaperStepChart::loadSettings() {
    settings_.beginGroup("StepChart");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        StepEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.step = settings_.value(QString("step_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.phase = settings_.value(QString("phase_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.baseline = settings_.value(QString("baseline_%1").arg(i)).toDouble();
        e.delta = settings_.value(QString("delta_%1").arg(i)).toDouble();
        e.improved = settings_.value(QString("improved_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperStepChart::saveSettings() {
    settings_.beginGroup("StepChart");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("step_%1").arg(i), e.step);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("phase_%1").arg(i), e.phase);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("baseline_%1").arg(i), e.baseline);
        settings_.setValue(QString("delta_%1").arg(i), e.delta);
        settings_.setValue(QString("improved_%1").arg(i), e.improved);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
