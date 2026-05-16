#include "visualization/PaperStepPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperStepPlot::PaperStepPlot(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "StepPlot") {
    setupUI();
    loadSettings();
}

void PaperStepPlot::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    renderBtn_ = new QPushButton("Render", this);
    renderBtn_->setStyleSheet("background:#3b82f6;color:white;padding:6px 14px;border-radius:4px;");
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Metric A", "Metric B", "Metric C", "Metric D"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Entry label...");
    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("background:#dc2626;color:white;padding:6px 14px;border-radius:4px;");
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    infoLabel_ = new QLabel("Entries: 0 | Rising: 0 | Max Value: 0.00", this);
    mainLayout->addWidget(infoLabel_);
    connect(renderBtn_, &QPushButton::clicked, this, &PaperStepPlot::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStepPlot::onClear);
}

void PaperStepPlot::addEntry(const StepEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<StepEntry> PaperStepPlot::entries() const { return entries_; }

int PaperStepPlot::risingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.rising) c++;
    return c;
}

qreal PaperStepPlot::maxValue() const {
    qreal mx = 0;
    for (const auto& e : entries_) mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperStepPlot::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperStepPlot::onRender() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int count = 5 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        StepEntry e;
        e.id = entries_.size() + 1;
        QString lbl = inputField_->text().trimmed();
        e.label = lbl.isEmpty() ? QString("Entry_%1").arg(e.id) : QString("%1_%2").arg(lbl).arg(e.id);
        QStringList cats = {"Metric A", "Metric B", "Metric C", "Metric D"};
        e.category = cats[QRandomGenerator::global()->bounded(cats.size())];
        QStringList axes = {"X", "Y", "Z"};
        e.axis = axes[QRandomGenerator::global()->bounded(axes.size())];
        e.value = QRandomGenerator::global()->bounded(0.0, 100.0);
        e.step = static_cast<qreal>(e.id);
        e.rising = e.value > 50.0;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        emit stepClicked(e.id, e.value);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperStepPlot::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperStepPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawStepChart(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryLegend(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperStepPlot::drawStepChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Step Plot:");
    if (entries_.isEmpty()) return;
    int stepW = qMax(10, (rect.width() - 20) / qMax(entries_.size(), 1));
    int baseY = rect.top() + rect.height() / 2 + 10;
    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::DashLine));
    p.drawLine(rect.left(), baseY, rect.right(), baseY);
    qreal mv = maxValue();
    if (mv <= 0) mv = 1.0;
    int x = rect.left();
    qreal prevVal = 0;
    for (int i = 0; i < qMin(entries_.size(), 20); ++i) {
        const auto& e = entries_[i];
        int y = baseY - static_cast<int>(e.value / mv * (rect.height() / 2 - 30));
        if (i > 0) {
            p.setPen(QPen(e.color, 2));
            int prevY = baseY - static_cast<int>(prevVal / mv * (rect.height() / 2 - 30));
            p.drawLine(x - stepW, prevY, x, prevY);
            p.drawLine(x, prevY, x, y);
        }
        p.setBrush(e.color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(x - 3, y - 3, 6, 6);
        p.setPen(QColor(0x334155));
        p.drawText(x - 10, baseY + 14, e.label.left(8));
        prevVal = e.value;
        x += stepW;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperStepPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
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
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperStepPlot::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total Entries: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Rising: %1").arg(risingCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Max Value: %1").arg(QString::number(maxValue(), 'f', 2)));
}

void PaperStepPlot::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Rising: %2 | Max Value: %3")
        .arg(entries_.size()).arg(risingCount())
        .arg(QString::number(maxValue(), 'f', 2)));
}

void PaperStepPlot::loadSettings() {
    settings_.beginGroup("StepPlot");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        StepEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.axis = settings_.value(QString("axis_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.step = settings_.value(QString("step_%1").arg(i)).toDouble();
        e.rising = settings_.value(QString("rising_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperStepPlot::saveSettings() {
    settings_.beginGroup("StepPlot");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("axis_%1").arg(i), e.axis);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("step_%1").arg(i), e.step);
        settings_.setValue(QString("rising_%1").arg(i), e.rising);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
