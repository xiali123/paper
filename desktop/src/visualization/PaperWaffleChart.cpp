#include "visualization/PaperWaffleChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperWaffleChart::PaperWaffleChart(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperWaffleChart::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Topics", "Methods", "Journals", "Authors", "Regions"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Category label...");
    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Categories: 0 | Dominant: 0 | Max %: 0.0", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(renderBtn_, &QPushButton::clicked, this, &PaperWaffleChart::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWaffleChart::onClear);
}

void PaperWaffleChart::addEntry(const WaffleEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<WaffleEntry> PaperWaffleChart::entries() const { return entries_; }

int PaperWaffleChart::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dominant) c++;
    return c;
}

qreal PaperWaffleChart::maxPercentage() const {
    qreal mx = 0;
    for (const auto& e : entries_) mx = qMax(mx, e.percentage);
    return mx;
}

QMap<QString, int> PaperWaffleChart::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperWaffleChart::onRender() {
    WaffleEntry e;
    e.id = entries_.size() + 1;
    e.label = inputField_->text().trimmed();
    if (e.label.isEmpty()) e.label = QString("Cat_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.group = QString("Group_%1").arg((e.id - 1) / 3 + 1);
    e.count = QRandomGenerator::global()->bounded(1, 100);
    e.percentage = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.cells = static_cast<int>(e.percentage);
    e.dominant = e.percentage > 50;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit waffleRendered(e.id, e.percentage);
    update();
}

void PaperWaffleChart::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperWaffleChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawWaffleView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryLegend(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperWaffleChart::drawWaffleView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Waffle Chart:");
    if (entries_.isEmpty()) return;
    int totalCells = 100;
    int cols = 10;
    int cellSize = qMin((rect.width() - 20) / cols, (rect.height() - 40) / (totalCells / cols));
    int startX = rect.left() + (rect.width() - cols * cellSize) / 2;
    int startY = rect.top() + 20;
    int cellIdx = 0;
    for (int i = 0; i < entries_.size() && cellIdx < totalCells; ++i) {
        const auto& e = entries_[i];
        for (int j = 0; j < e.cells && cellIdx < totalCells; ++j) {
            int col = cellIdx % cols;
            int row = cellIdx / cols;
            p.setBrush(e.color);
            p.setPen(QColor(0xffffff));
            p.drawRoundedRect(startX + col * cellSize + 1, startY + row * cellSize + 1, cellSize - 2, cellSize - 2, 2, 2);
            cellIdx++;
        }
    }
    // Fill remaining with gray
    p.setBrush(QColor(0xe2e8f0));
    p.setPen(QColor(0xffffff));
    while (cellIdx < totalCells) {
        int col = cellIdx % cols;
        int row = cellIdx / cols;
        p.drawRoundedRect(startX + col * cellSize + 1, startY + row * cellSize + 1, cellSize - 2, cellSize - 2, 2, 2);
        cellIdx++;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperWaffleChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
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

void PaperWaffleChart::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Categories: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Dominant: %1").arg(dominantCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Max %: %1").arg(QString::number(maxPercentage(), 'f', 1)));
}

void PaperWaffleChart::updateInfo() {
    infoLabel_->setText(QString("Categories: %1 | Dominant: %2 | Max %: %3")
        .arg(entries_.size()).arg(dominantCount())
        .arg(QString::number(maxPercentage(), 'f', 1)));
}

void PaperWaffleChart::loadSettings() {
    settings_.beginGroup("WaffleChart");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        WaffleEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.group = settings_.value(QString("group_%1").arg(i)).toString();
        e.count = settings_.value(QString("count_%1").arg(i)).toInt();
        e.percentage = settings_.value(QString("percentage_%1").arg(i)).toDouble();
        e.cells = settings_.value(QString("cells_%1").arg(i)).toInt();
        e.dominant = settings_.value(QString("dominant_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperWaffleChart::saveSettings() {
    settings_.beginGroup("WaffleChart");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("group_%1").arg(i), e.group);
        settings_.setValue(QString("count_%1").arg(i), e.count);
        settings_.setValue(QString("percentage_%1").arg(i), e.percentage);
        settings_.setValue(QString("cells_%1").arg(i), e.cells);
        settings_.setValue(QString("dominant_%1").arg(i), e.dominant);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
