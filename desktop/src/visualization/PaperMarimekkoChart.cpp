#include "visualization/PaperMarimekkoChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMarimekkoChart::PaperMarimekkoChart(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperMarimekkoChart::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Fields", "Journals", "Authors", "Regions", "Years"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Segment name...");
    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Segments: 0 | Dominant: 0 | Total Area: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(renderBtn_, &QPushButton::clicked, this, &PaperMarimekkoChart::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMarimekkoChart::onClear);
}

void PaperMarimekkoChart::addEntry(const MekkoEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<MekkoEntry> PaperMarimekkoChart::entries() const { return entries_; }

int PaperMarimekkoChart::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dominant) c++;
    return c;
}

qreal PaperMarimekkoChart::totalArea() const {
    qreal s = 0;
    for (const auto& e : entries_) s += e.area;
    return s;
}

QMap<QString, int> PaperMarimekkoChart::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperMarimekkoChart::onRender() {
    MekkoEntry e;
    e.id = entries_.size() + 1;
    e.segment = inputField_->text().trimmed();
    if (e.segment.isEmpty()) e.segment = QString("Seg_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.axis = (e.id % 2 == 0) ? "X" : "Y";
    e.width = QRandomGenerator::global()->bounded(10.0, 100.0);
    e.height = QRandomGenerator::global()->bounded(10.0, 100.0);
    e.area = e.width * e.height;
    e.dominant = e.area > 5000;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit mekkRendered(e.id, e.area);
    update();
}

void PaperMarimekkoChart::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperMarimekkoChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawMekkoView(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryLegend(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperMarimekkoChart::drawMekkoView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Marimekko Chart:");
    if (entries_.isEmpty()) return;
    qreal totalW = 0;
    for (const auto& e : entries_) totalW += e.width;
    if (totalW <= 0) totalW = 1;
    int x = rect.left();
    int maxH = rect.height() - 30;
    for (int i = 0; i < qMin(entries_.size(), 10); ++i) {
        const auto& e = entries_[i];
        int w = static_cast<int>(e.width / totalW * rect.width());
        int h = static_cast<int>(e.height / 100.0 * maxH);
        w = qMax(w, 5);
        h = qMax(h, 5);
        if (x + w > rect.right()) break;
        QColor c = e.color;
        c.setAlpha(180);
        p.setBrush(c);
        p.setPen(QColor(0xffffff));
        p.drawRect(x, rect.top() + 20 + maxH - h, w, h);
        p.setPen(QColor(0x334155));
        if (w > 25) p.drawText(x + 2, rect.top() + 20 + maxH - h + 12, e.segment.left(w / 7));
        x += w;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperMarimekkoChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
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

void PaperMarimekkoChart::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Segments: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Dominant: %1").arg(dominantCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Total Area: %1").arg(QString::number(totalArea(), 'f', 0)));
}

void PaperMarimekkoChart::updateInfo() {
    infoLabel_->setText(QString("Segments: %1 | Dominant: %2 | Total Area: %3")
        .arg(entries_.size()).arg(dominantCount())
        .arg(QString::number(totalArea(), 'f', 0)));
}

void PaperMarimekkoChart::loadSettings() {
    settings_.beginGroup("MarimekkoChart");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        MekkoEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.segment = settings_.value(QString("segment_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.axis = settings_.value(QString("axis_%1").arg(i)).toString();
        e.width = settings_.value(QString("width_%1").arg(i)).toDouble();
        e.height = settings_.value(QString("height_%1").arg(i)).toDouble();
        e.area = settings_.value(QString("area_%1").arg(i)).toDouble();
        e.dominant = settings_.value(QString("dominant_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperMarimekkoChart::saveSettings() {
    settings_.beginGroup("MarimekkoChart");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("segment_%1").arg(i), e.segment);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("axis_%1").arg(i), e.axis);
        settings_.setValue(QString("width_%1").arg(i), e.width);
        settings_.setValue(QString("height_%1").arg(i), e.height);
        settings_.setValue(QString("area_%1").arg(i), e.area);
        settings_.setValue(QString("dominant_%1").arg(i), e.dominant);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
