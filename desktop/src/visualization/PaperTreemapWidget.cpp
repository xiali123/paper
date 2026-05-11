#include "visualization/PaperTreemapWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTreemapWidget::PaperTreemapWidget(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperTreemapWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Topics", "Authors", "Journals", "Keywords", "Methods"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Node label...");
    buildBtn_ = new QPushButton("Build", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Nodes: 0 | Leaves: 0 | Total Value: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(buildBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(buildBtn_, &QPushButton::clicked, this, &PaperTreemapWidget::onBuild);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTreemapWidget::onClear);
}

void PaperTreemapWidget::addEntry(const TreeEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<TreeEntry> PaperTreemapWidget::entries() const { return entries_; }

int PaperTreemapWidget::leafCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.leaf) c++;
    return c;
}

qreal PaperTreemapWidget::totalValue() const {
    qreal s = 0;
    for (const auto& e : entries_) s += e.value;
    return s;
}

QMap<QString, int> PaperTreemapWidget::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperTreemapWidget::onBuild() {
    TreeEntry e;
    e.id = entries_.size() + 1;
    e.label = inputField_->text().trimmed();
    if (e.label.isEmpty()) e.label = QString("Node_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.parent = entries_.isEmpty() ? "Root" : QString("Node_%1").arg(qMax(1, e.id - 1));
    e.value = QRandomGenerator::global()->bounded(1.0, 100.0);
    e.area = e.value;
    e.depth = QRandomGenerator::global()->bounded(0, 4);
    e.leaf = e.depth == 3 || QRandomGenerator::global()->bounded(3) == 0;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit treemapBuilt(e.id, e.value);
    update();
}

void PaperTreemapWidget::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperTreemapWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawTreemapView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryLegend(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperTreemapWidget::drawTreemapView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Treemap View:");
    if (entries_.isEmpty()) return;
    qreal total = totalValue();
    if (total <= 0) total = 1;
    int x = rect.left(), y = rect.top() + 20;
    int remaining = rect.width();
    for (int i = 0; i < qMin(entries_.size(), 12); ++i) {
        const auto& e = entries_[i];
        int w = static_cast<int>(e.value / total * rect.width());
        int h = static_cast<int>(e.value / total * (rect.height() - 30));
        h = qMax(h, 15);
        w = qMax(w, 20);
        if (x + w > rect.right()) break;
        QColor c = e.color;
        c.setAlpha(160);
        p.setBrush(c);
        p.setPen(QColor(0xffffff));
        p.drawRect(x, y, w, h);
        p.setPen(QColor(0xffffff));
        if (w > 30) p.drawText(x + 3, y + 12, e.label.left(w / 7));
        x += w;
        if (x > rect.right() - 20) {
            x = rect.left();
            y += h;
        }
    }
    p.setBrush(Qt::NoBrush);
}

void PaperTreemapWidget::drawCategoryLegend(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:");
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

void PaperTreemapWidget::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total Nodes: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Leaves: %1").arg(leafCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Total Value: %1").arg(QString::number(totalValue(), 'f', 1)));
}

void PaperTreemapWidget::updateInfo() {
    infoLabel_->setText(QString("Nodes: %1 | Leaves: %2 | Total Value: %3")
        .arg(entries_.size()).arg(leafCount())
        .arg(QString::number(totalValue(), 'f', 1)));
}

void PaperTreemapWidget::loadSettings() {
    settings_.beginGroup("TreemapWidget");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        TreeEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.parent = settings_.value(QString("parent_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.area = settings_.value(QString("area_%1").arg(i)).toDouble();
        e.depth = settings_.value(QString("depth_%1").arg(i)).toInt();
        e.leaf = settings_.value(QString("leaf_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperTreemapWidget::saveSettings() {
    settings_.beginGroup("TreemapWidget");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("parent_%1").arg(i), e.parent);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("area_%1").arg(i), e.area);
        settings_.setValue(QString("depth_%1").arg(i), e.depth);
        settings_.setValue(QString("leaf_%1").arg(i), e.leaf);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
