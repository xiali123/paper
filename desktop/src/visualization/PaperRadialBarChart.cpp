#include "visualization/PaperRadialBarChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperRadialBarChart::PaperRadialBarChart(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "RadialBarChart") {
    setupUI();
    loadSettings();
}

void PaperRadialBarChart::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    renderBtn_ = new QPushButton("Render", this);
    renderBtn_->setStyleSheet("QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;padding:6px 16px;}"
                              "QPushButton:hover{background:#2563eb;}");
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Citations", "Impact", "Collaboration", "Diversity"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Entry label...");
    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;padding:6px 16px;}"
                             "QPushButton:hover{background:#b91c1c;}");
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    infoLabel_ = new QLabel("Entries: 0 | Filled: 0 | Avg Fill: 0.0%", this);
    mainLayout->addWidget(infoLabel_);
    connect(renderBtn_, &QPushButton::clicked, this, &PaperRadialBarChart::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRadialBarChart::onClear);
}

void PaperRadialBarChart::addEntry(const RadialEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<RadialEntry> PaperRadialBarChart::entries() const { return entries_; }

int PaperRadialBarChart::filledCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.filled) c++;
    return c;
}

qreal PaperRadialBarChart::avgFill() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += (e.max > 0 ? e.value / e.max : 0);
    return sum / entries_.size() * 100.0;
}

QMap<QString, int> PaperRadialBarChart::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperRadialBarChart::onRender() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int count = QRandomGenerator::global()->bounded(5, 11);
    QStringList categories = {"Citations", "Impact", "Collaboration", "Diversity"};
    QStringList rings = {"Inner", "Middle", "Outer"};
    for (int i = 0; i < count; ++i) {
        RadialEntry e;
        e.id = entries_.size() + 1;
        e.label = inputField_->text().trimmed().isEmpty()
            ? QString("Entry_%1").arg(e.id)
            : QString("%1_%2").arg(inputField_->text().trimmed()).arg(e.id);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.ring = rings[QRandomGenerator::global()->bounded(rings.size())];
        e.max = 100.0;
        e.value = QRandomGenerator::global()->bounded(1.0, 100.0);
        e.filled = e.value / e.max >= 0.75;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperRadialBarChart::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperRadialBarChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawRadialChart(p, QRect(10, 50, w / 2, h - 60));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 20, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperRadialBarChart::drawRadialChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Radial Bar Chart:");
    if (entries_.isEmpty()) return;
    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int maxR = qMin(rect.width(), rect.height()) / 2 - 30;
    int n = entries_.size();
    qreal barHeight = qMax(static_cast<qreal>(maxR) / n, 4.0);
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        qreal ratio = e.max > 0 ? e.value / e.max : 0;
        int rInner = static_cast<int>(maxR - (i + 1) * barHeight);
        int rOuter = static_cast<int>(maxR - i * barHeight);
        int barW = static_cast<int>((maxR - 40) * ratio);
        QRect barRect(cx - barW / 2, cy - rOuter, barW, rOuter - rInner);
        QColor c = e.color;
        c.setAlpha(e.filled ? 210 : 120);
        p.setBrush(c);
        p.setPen(QColor(0xffffff));
        p.drawRoundedRect(barRect, 3, 3);
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 7));
        p.drawText(cx + barW / 2 + 4, cy - (rInner + rOuter) / 2 + 4, e.label);
    }
    p.setBrush(Qt::NoBrush);
}

void PaperRadialBarChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
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
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 25, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperRadialBarChart::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Entries: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Filled: %1").arg(filledCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Fill: %1%").arg(QString::number(avgFill(), 'f', 1)));
}

void PaperRadialBarChart::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Filled: %2 | Avg Fill: %3%")
        .arg(entries_.size()).arg(filledCount())
        .arg(QString::number(avgFill(), 'f', 1)));
}

void PaperRadialBarChart::loadSettings() {
    settings_.beginGroup("RadialBarChart");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        RadialEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.ring = settings_.value(QString("ring_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.max = settings_.value(QString("max_%1").arg(i)).toDouble();
        e.filled = settings_.value(QString("filled_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperRadialBarChart::saveSettings() {
    settings_.beginGroup("RadialBarChart");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("ring_%1").arg(i), e.ring);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("max_%1").arg(i), e.max);
        settings_.setValue(QString("filled_%1").arg(i), e.filled);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
