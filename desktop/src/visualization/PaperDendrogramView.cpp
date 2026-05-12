#include "visualization/PaperDendrogramView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDendrogramView::PaperDendrogramView(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperDendrogramView::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Ward", "Complete", "Average", "Single", "Centroid"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Cluster label...");
    buildBtn_ = new QPushButton("Build", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Nodes: 0 | Leaves: 0 | Max Height: 0.00", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(buildBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(buildBtn_, &QPushButton::clicked, this, &PaperDendrogramView::onBuild);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDendrogramView::onClear);
}

void PaperDendrogramView::addEntry(const DendroEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<DendroEntry> PaperDendrogramView::entries() const { return entries_; }
int PaperDendrogramView::leafCount() const { int c = 0; for (const auto& e : entries_) if (e.leaf) c++; return c; }
qreal PaperDendrogramView::maxHeight() const { qreal mx = 0; for (const auto& e : entries_) mx = qMax(mx, e.height); return mx; }
QMap<QString, int> PaperDendrogramView::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperDendrogramView::onBuild() {
    DendroEntry e;
    e.id = entries_.size() + 1;
    e.cluster = inputField_->text().trimmed();
    if (e.cluster.isEmpty()) e.cluster = QString("Cl_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.method = categoryCombo_->currentText();
    e.height = QRandomGenerator::global()->bounded(0.0, 10.0);
    e.distance = QRandomGenerator::global()->bounded(0.0, 5.0);
    e.members = QRandomGenerator::global()->bounded(1, 100);
    e.leaf = e.members < 5;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e); updateInfo(); saveSettings();
    emit dendroBuilt(e.id, e.height); update();
}

void PaperDendrogramView::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperDendrogramView::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));
    drawDendroView(p, QRect(10, 50, width() - 20, height() / 2 - 60));
    drawCategoryLegend(p, QRect(10, height() / 2, width() / 2 - 10, height() / 2 - 60));
    drawStats(p, QRect(width() / 2 + 10, height() / 2, width() / 2 - 20, height() / 2 - 60));
}

void PaperDendrogramView::drawDendroView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Dendrogram:");
    if (entries_.isEmpty()) return;
    qreal mh = maxHeight(); if (mh <= 0) mh = 1;
    int baseY = rect.bottom() - 10;
    int topY = rect.top() + 20;
    int n = qMin(entries_.size(), 12);
    int spacing = (rect.width() - 20) / qMax(n, 1);
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int x = rect.left() + 10 + i * spacing + spacing / 2;
        int y = baseY - static_cast<int>(e.height / mh * (baseY - topY));
        p.setPen(QPen(e.color, 2));
        p.drawLine(x, baseY, x, y);
        if (i < n - 1) {
            int nextY = baseY - static_cast<int>(entries_[i + 1].height / mh * (baseY - topY));
            p.drawLine(x, y, x + spacing, nextY);
        }
        p.setBrush(e.leaf ? e.color : QColor(0xe2e8f0));
        p.setPen(QColor(0xffffff));
        p.drawEllipse(x - 4, y - 4, 8, 8);
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 7));
        p.drawText(x - 15, baseY + 12, e.cluster.left(5));
    }
    p.setBrush(Qt::NoBrush);
}

void PaperDendrogramView::drawCategoryLegend(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Method:"); y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155)); p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value())); y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperDendrogramView::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Nodes: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Leaves: %1").arg(leafCount())); y += 16;
    p.drawText(rect.left(), y, QString("Max Height: %1").arg(QString::number(maxHeight(), 'f', 2)));
}

void PaperDendrogramView::updateInfo() {
    infoLabel_->setText(QString("Nodes: %1 | Leaves: %2 | Max Height: %3")
        .arg(entries_.size()).arg(leafCount()).arg(QString::number(maxHeight(), 'f', 2)));
}

void PaperDendrogramView::loadSettings() {
    settings_.beginGroup("DendrogramView");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        DendroEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.cluster = settings_.value(QString("cluster_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.method = settings_.value(QString("method_%1").arg(i)).toString();
        e.height = settings_.value(QString("height_%1").arg(i)).toDouble();
        e.distance = settings_.value(QString("distance_%1").arg(i)).toDouble();
        e.members = settings_.value(QString("members_%1").arg(i)).toInt();
        e.leaf = settings_.value(QString("leaf_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperDendrogramView::saveSettings() {
    settings_.beginGroup("DendrogramView"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("cluster_%1").arg(i), e.cluster);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("method_%1").arg(i), e.method);
        settings_.setValue(QString("height_%1").arg(i), e.height);
        settings_.setValue(QString("distance_%1").arg(i), e.distance);
        settings_.setValue(QString("members_%1").arg(i), e.members);
        settings_.setValue(QString("leaf_%1").arg(i), e.leaf);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
