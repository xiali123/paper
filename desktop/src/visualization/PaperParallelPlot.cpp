#include "visualization/PaperParallelPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperParallelPlot::PaperParallelPlot(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperParallelPlot::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Dimensions", "Metrics", "Features", "Scores", "Attributes"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Entry label...");
    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Entries: 0 | Outliers: 0 | Avg Range: 0.00", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(renderBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(renderBtn_, &QPushButton::clicked, this, &PaperParallelPlot::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperParallelPlot::onClear);
}

void PaperParallelPlot::addEntry(const ParallelEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<ParallelEntry> PaperParallelPlot::entries() const { return entries_; }

int PaperParallelPlot::outlierCount() const { int c = 0; for (const auto& e : entries_) if (e.outlier) c++; return c; }

qreal PaperParallelPlot::avgRange() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0; for (const auto& e : entries_) sum += (qMax(e.x1, qMax(e.x2, e.x3)) - qMin(e.x1, qMin(e.x2, e.x3)));
    return sum / entries_.size();
}

QMap<QString, int> PaperParallelPlot::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperParallelPlot::onRender() {
    ParallelEntry e;
    e.id = entries_.size() + 1;
    e.label = inputField_->text().trimmed();
    if (e.label.isEmpty()) e.label = QString("Entry_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.axis = QString("Axis_%1").arg((e.id - 1) / 3 + 1);
    e.x1 = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.x2 = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.x3 = QRandomGenerator::global()->bounded(0.0, 100.0);
    qreal range = qMax(e.x1, qMax(e.x2, e.x3)) - qMin(e.x1, qMin(e.x2, e.x3));
    e.outlier = range > 80;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo(); saveSettings();
    emit parallelRendered(e.id, range);
    update();
}

void PaperParallelPlot::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperParallelPlot::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawParallelView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryLegend(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperParallelPlot::drawParallelView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Parallel Coordinates:");
    if (entries_.isEmpty()) return;
    int x1 = rect.left() + 20, x2 = rect.left() + rect.width() / 2, x3 = rect.right() - 20;
    // Draw axes
    p.setPen(QPen(QColor(0x94a3b8), 1));
    p.drawLine(x1, rect.top() + 20, x1, rect.bottom());
    p.drawLine(x2, rect.top() + 20, x2, rect.bottom());
    p.drawLine(x3, rect.top() + 20, x3, rect.bottom());
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 8));
    p.drawText(x1 - 10, rect.top() + 16, "X1"); p.drawText(x2 - 10, rect.top() + 16, "X2"); p.drawText(x3 - 10, rect.top() + 16, "X3");
    int h = rect.height() - 30;
    for (int i = 0; i < qMin(entries_.size(), 10); ++i) {
        const auto& e = entries_[i];
        int y1 = rect.top() + 20 + static_cast<int>((1 - e.x1 / 100.0) * h);
        int y2 = rect.top() + 20 + static_cast<int>((1 - e.x2 / 100.0) * h);
        int y3 = rect.top() + 20 + static_cast<int>((1 - e.x3 / 100.0) * h);
        p.setPen(QPen(e.color, e.outlier ? 2 : 1));
        p.drawLine(x1, y1, x2, y2); p.drawLine(x2, y2, x3, y3);
    }
}

void PaperParallelPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Group:"); y += 18;
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

void PaperParallelPlot::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Entries: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Outliers: %1").arg(outlierCount())); y += 16;
    p.drawText(rect.left(), y, QString("Avg Range: %1").arg(QString::number(avgRange(), 'f', 1)));
}

void PaperParallelPlot::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Outliers: %2 | Avg Range: %3")
        .arg(entries_.size()).arg(outlierCount()).arg(QString::number(avgRange(), 'f', 2)));
}

void PaperParallelPlot::loadSettings() {
    settings_.beginGroup("ParallelPlot");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ParallelEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.axis = settings_.value(QString("axis_%1").arg(i)).toString();
        e.x1 = settings_.value(QString("x1_%1").arg(i)).toDouble();
        e.x2 = settings_.value(QString("x2_%1").arg(i)).toDouble();
        e.x3 = settings_.value(QString("x3_%1").arg(i)).toDouble();
        e.outlier = settings_.value(QString("outlier_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperParallelPlot::saveSettings() {
    settings_.beginGroup("ParallelPlot"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("axis_%1").arg(i), e.axis);
        settings_.setValue(QString("x1_%1").arg(i), e.x1);
        settings_.setValue(QString("x2_%1").arg(i), e.x2);
        settings_.setValue(QString("x3_%1").arg(i), e.x3);
        settings_.setValue(QString("outlier_%1").arg(i), e.outlier);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
