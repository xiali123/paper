#include "reading/PaperReadingCompass.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingCompass::PaperReadingCompass(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperReadingCompass::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Theory", "Application", "Survey", "Experimental", "Review"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");
    navigateBtn_ = new QPushButton("Navigate", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Papers: 0 | On Course: 0 | Avg Depth: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(navigateBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(navigateBtn_, &QPushButton::clicked, this, &PaperReadingCompass::onNavigate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingCompass::onClear);
}

void PaperReadingCompass::addEntry(const CompassEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<CompassEntry> PaperReadingCompass::entries() const { return entries_; }

int PaperReadingCompass::onCourseCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.onCourse) c++;
    return c;
}

qreal PaperReadingCompass::avgDepth() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.depth;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingCompass::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingCompass::onNavigate() {
    CompassEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList dirs = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    e.direction = dirs[QRandomGenerator::global()->bounded(dirs.size())];
    e.relevance = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.depth = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.breadth = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.onCourse = e.relevance > 0.5 && e.depth > 0.4;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit compassUpdated(e.id, e.relevance);
    update();
}

void PaperReadingCompass::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingCompass::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawCompassView(p, QRect(10, 50, w / 2, h - 60));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 20, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperReadingCompass::drawCompassView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reading Compass:");
    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;
    // Draw compass circles
    p.setPen(QPen(QColor(0xe2e8f0), 1));
    for (int r = 1; r <= 3; ++r) {
        p.drawEllipse(cx - radius * r / 3, cy - radius * r / 3, radius * r * 2 / 3, radius * r * 2 / 3);
    }
    // Draw axis
    p.setPen(QPen(QColor(0x94a3b8), 1));
    p.drawLine(cx - radius, cy, cx + radius, cy);
    p.drawLine(cx, cy - radius, cx, cy + radius);
    // Draw points
    QStringList dirs = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    qreal dirAngles[8] = {-M_PI/2, -M_PI/4, 0, M_PI/4, M_PI/2, 3*M_PI/4, M_PI, -3*M_PI/4};
    p.setFont(QFont("Sans", 8));
    for (int i = 0; i < 8; ++i) {
        p.setPen(QColor(0x94a3b8));
        p.drawText(cx + static_cast<int>((radius + 12) * qCos(dirAngles[i])) - 6, cy + static_cast<int>((radius + 12) * qSin(dirAngles[i])) + 4, dirs[i]);
    }
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        int idx = dirs.indexOf(e.direction);
        if (idx < 0) idx = 0;
        qreal angle = dirAngles[idx];
        qreal r = e.relevance * radius * 0.9;
        int px = cx + static_cast<int>(r * qCos(angle));
        int py = cy + static_cast<int>(r * qSin(angle));
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawEllipse(px - 4, py - 4, 8, 8);
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingCompass::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperReadingCompass::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("On Course: %1").arg(onCourseCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Depth: %1").arg(QString::number(avgDepth(), 'f', 3)));
}

void PaperReadingCompass::updateInfo() {
    infoLabel_->setText(QString("Papers: %1 | On Course: %2 | Avg Depth: %3")
        .arg(entries_.size()).arg(onCourseCount())
        .arg(QString::number(avgDepth(), 'f', 2)));
}

void PaperReadingCompass::loadSettings() {
    settings_.beginGroup("ReadingCompass");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        CompassEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.direction = settings_.value(QString("direction_%1").arg(i)).toString();
        e.relevance = settings_.value(QString("relevance_%1").arg(i)).toDouble();
        e.depth = settings_.value(QString("depth_%1").arg(i)).toDouble();
        e.breadth = settings_.value(QString("breadth_%1").arg(i)).toDouble();
        e.onCourse = settings_.value(QString("onCourse_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingCompass::saveSettings() {
    settings_.beginGroup("ReadingCompass");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("direction_%1").arg(i), e.direction);
        settings_.setValue(QString("relevance_%1").arg(i), e.relevance);
        settings_.setValue(QString("depth_%1").arg(i), e.depth);
        settings_.setValue(QString("breadth_%1").arg(i), e.breadth);
        settings_.setValue(QString("onCourse_%1").arg(i), e.onCourse);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
