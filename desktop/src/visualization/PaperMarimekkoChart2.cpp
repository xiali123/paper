#include "visualization/PaperMarimekkoChart2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMarimekkoChart2::PaperMarimekkoChart2(QWidget* parent)
    : QWidget(parent), settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperMarimekkoChart2") {
    setupUI();
    loadSettings();
}

void PaperMarimekkoChart2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    auto* left = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Fields", "Journals", "Authors", "Regions", "Years"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter label...");
    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Segments: 0 | Dominant: 0 | Max: 0.0", this);
    left->addWidget(categoryCombo_);
    left->addWidget(inputField_);
    left->addWidget(renderBtn_);
    left->addWidget(clearBtn_);
    left->addWidget(infoLabel_);
    mainLayout->addLayout(left);
    mainLayout->addStretch();
    connect(renderBtn_, &QPushButton::clicked, this, &PaperMarimekkoChart2::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMarimekkoChart2::onClear);
}

void PaperMarimekkoChart2::addEntry(const MarimekkoEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<MarimekkoEntry> PaperMarimekkoChart2::entries() const { return entries_; }

int PaperMarimekkoChart2::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dominant) c++;
    return c;
}

qreal PaperMarimekkoChart2::maxValue() const {
    qreal m = 0;
    for (const auto& e : entries_) if (e.value > m) m = e.value;
    return m;
}

QMap<QString, int> PaperMarimekkoChart2::categoryCounts() const {
    QMap<QString, int> map;
    for (const auto& e : entries_) map[e.category]++;
    return map;
}

void PaperMarimekkoChart2::onRender() {
    MarimekkoEntry e;
    e.id = entries_.size() + 1;
    e.label = inputField_->text().trimmed();
    if (e.label.isEmpty()) e.label = QString("Seg_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.width = 0.2 + QRandomGenerator::global()->bounded(0.8);
    e.height = 0.2 + QRandomGenerator::global()->bounded(0.8);
    e.value = e.width * e.height * 100.0;
    e.dominant = e.value > 50.0;
    QList<QString> names = {"Alpha", "Beta", "Gamma", "Delta", "Epsilon"};
    e.segment = names[QRandomGenerator::global()->bounded(names.size())];
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit segmentSelected(e.id, e.value);
    update();
}

void PaperMarimekkoChart2::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperMarimekkoChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    int colW = w / 3;
    drawMarimekkoView(p, QRect(10, 10, colW - 15, h - 20));
    drawCategoryLegend(p, QRect(colW + 5, 10, colW - 15, h - 20));
    drawStats(p, QRect(2 * colW + 5, 10, colW - 15, h - 20));
}

void PaperMarimekkoChart2::drawMarimekkoView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Marimekko Chart");
    if (entries_.isEmpty()) return;
    qreal totalW = 0;
    for (const auto& e : entries_) totalW += e.width;
    if (totalW <= 0) totalW = 1;
    int x = rect.left();
    int drawTop = rect.top() + 25;
    int maxH = rect.height() - 35;
    for (int i = 0; i < qMin(entries_.size(), 12); ++i) {
        const auto& e = entries_[i];
        int rw = static_cast<int>(e.width / totalW * rect.width());
        int rh = static_cast<int>(e.height * maxH);
        rw = qMax(rw, 5);
        rh = qMax(rh, 5);
        if (x + rw > rect.right()) break;
        QColor c = e.color;
        c.setAlpha(204); // 80% alpha
        p.setBrush(c);
        p.setPen(QColor(0xffffff));
        p.drawRect(x, drawTop + maxH - rh, rw, rh);
        p.setPen(QColor(0xffffff));
        p.setFont(QFont("Sans", 8));
        if (rw > 30 && rh > 20) {
            p.drawText(QRect(x + 2, drawTop + maxH - rh + 2, rw - 4, rh - 4),
                       Qt::AlignCenter, QString::number(e.value, 'f', 1));
        }
        x += rw;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperMarimekkoChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Legend");
    y += 25;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (y + 22 > rect.bottom()) break;
        p.setBrush(colors[ci++ % colors.size()]);
        p.setPen(Qt::NoPen);
        p.drawRect(rect.left(), y, 14, 14);
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.left() + 20, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperMarimekkoChart2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");
    int y = rect.top() + 25;
    p.setFont(QFont("Sans", 9));
    p.drawText(rect.left(), y, QString("Total segments: %1").arg(entries_.size()));
    y += 18;
    p.drawText(rect.left(), y, QString("Dominant count: %1").arg(dominantCount()));
    y += 18;
    p.drawText(rect.left(), y, QString("Max value: %1").arg(QString::number(maxValue(), 'f', 1)));
}

void PaperMarimekkoChart2::updateInfo() {
    infoLabel_->setText(QString("Segments: %1 | Dominant: %2 | Max: %3")
        .arg(entries_.size()).arg(dominantCount())
        .arg(QString::number(maxValue(), 'f', 1)));
}

void PaperMarimekkoChart2::loadSettings() {
    settings_.beginGroup("MarimekkoChart2");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        MarimekkoEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.segment = settings_.value(QString("segment_%1").arg(i)).toString();
        e.width = settings_.value(QString("width_%1").arg(i)).toDouble();
        e.height = settings_.value(QString("height_%1").arg(i)).toDouble();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.dominant = settings_.value(QString("dominant_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperMarimekkoChart2::saveSettings() {
    settings_.beginGroup("MarimekkoChart2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("segment_%1").arg(i), e.segment);
        settings_.setValue(QString("width_%1").arg(i), e.width);
        settings_.setValue(QString("height_%1").arg(i), e.height);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("dominant_%1").arg(i), e.dominant);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
