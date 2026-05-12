#include "reading/PaperReadingVortex.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingVortex::PaperReadingVortex(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingVortex") {
    setupUI();
    loadSettings();
}

void PaperReadingVortex::setupUI() {
    auto* toolbar = new QHBoxLayout();

    spinBtn_ = new QPushButton("Spin", this);
    spinBtn_->setStyleSheet("QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;padding:6px 16px;font-weight:bold;}");

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"Theory", "Application", "Survey", "Experimental", "Review"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;padding:6px 16px;font-weight:bold;}");

    toolbar->addWidget(spinBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();

    infoLabel_ = new QLabel("Entries: 0 | Diverging: 0 | Avg Velocity: 0.00", this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    mainLayout->addStretch();

    connect(spinBtn_, &QPushButton::clicked, this, &PaperReadingVortex::onSpin);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingVortex::onClear);
}

void PaperReadingVortex::onSpin() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList categories = {"Theory", "Application", "Survey", "Experimental", "Review"};
    QStringList dirs = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

    int count = QRandomGenerator::global()->bounded(3, 7);
    for (int i = 0; i < count; ++i) {
        VortexEntry e;
        e.id = entries_.size() + 1;
        e.paper = inputField_->text().trimmed();
        if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.direction = dirs[QRandomGenerator::global()->bounded(dirs.size())];
        e.velocity = QRandomGenerator::global()->generateDouble() * 4.0 + 0.5;
        e.rotations = QRandomGenerator::global()->bounded(1, 13);
        e.diverging = QRandomGenerator::global()->generateDouble() > 0.6;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        emit vortexSpin(e.id, e.velocity);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingVortex::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingVortex::addEntry(const VortexEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<VortexEntry> PaperReadingVortex::entries() const {
    return entries_;
}

int PaperReadingVortex::divergingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.diverging) c++;
    return c;
}

qreal PaperReadingVortex::avgVelocity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.velocity;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingVortex::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingVortex::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    int colW = w / 3;
    drawVortexDiagram(p, QRect(10, 10, colW - 20, h - 20));
    drawCategoryChart(p, QRect(colW + 10, 10, colW - 20, h - 20));
    drawStats(p, QRect(2 * colW + 10, 10, colW - 20, h - 20));
}

void PaperReadingVortex::drawVortexDiagram(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reading Vortex");

    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int radius = qMin(rect.width(), rect.height()) / 2 - 40;

    for (int r = 1; r <= 4; ++r) {
        int ringR = radius * r / 4;
        p.setPen(QPen(QColor(0xe2e8f0), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cx - ringR, cy - ringR, ringR * 2, ringR * 2);
    }

    p.setPen(QPen(QColor(0x94a3b8), 1, Qt::DashLine));
    p.drawLine(cx - radius, cy, cx + radius, cy);
    p.drawLine(cx, cy - radius, cx, cy + radius);

    for (const auto& e : entries_) {
        qreal angle = (e.id * 137.508 + e.rotations * 30.0) * M_PI / 180.0;
        qreal r = e.velocity / 4.5 * radius * 0.9;
        int px = cx + static_cast<int>(r * qCos(angle));
        int py = cy + static_cast<int>(r * qSin(angle));

        int dotSize = e.rotations;
        dotSize = qBound(4, dotSize, 12);

        if (e.diverging) {
            p.setPen(QPen(e.color, 1));
            p.setBrush(QColor(e.color.red(), e.color.green(), e.color.blue(), 100));
            p.drawEllipse(px - dotSize - 2, py - dotSize - 2, (dotSize + 2) * 2, (dotSize + 2) * 2);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(px - dotSize / 2, py - dotSize / 2, dotSize, dotSize);

        qreal spiralAngle = angle - 0.3;
        int spx = cx + static_cast<int>((r + 8) * qCos(spiralAngle));
        int spy = cy + static_cast<int>((r + 8) * qSin(spiralAngle));
        p.setPen(QPen(QColor(e.color.red(), e.color.green(), e.color.blue(), 120), 1));
        p.drawLine(px, py, spx, spy);
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingVortex::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor(0x94a3b8));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.adjusted(0, 20, 0, 0), Qt::AlignLeft | Qt::AlignTop, "No data yet");
        return;
    }

    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxCount = qMax(maxCount, it.value());

    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    int y = rect.top() + 24;
    int barMaxW = rect.width() - 100;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QColor barColor = colors[ci++ % colors.size()];
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.left(), y + 12, it.key());

        int barW = maxCount > 0 ? (it.value() * barMaxW / maxCount) : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.left() + 80, y, barW, 16, 3, 3);

        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 80 + barW + 6, y + 13, QString::number(it.value()));
        y += 24;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingVortex::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int y = rect.top() + 28;
    p.setFont(QFont("Sans", 9));

    p.drawText(rect.left(), y, QString("Total Entries: %1").arg(entries_.size()));
    y += 22;

    p.drawText(rect.left(), y, QString("Diverging: %1").arg(divergingCount()));
    y += 22;

    p.drawText(rect.left(), y, QString("Avg Velocity: %1").arg(QString::number(avgVelocity(), 'f', 2)));
    y += 22;

    if (!entries_.isEmpty()) {
        int totalRot = 0;
        for (const auto& e : entries_) totalRot += e.rotations;
        p.drawText(rect.left(), y, QString("Avg Rotations: %1").arg(QString::number(static_cast<qreal>(totalRot) / entries_.size(), 'f', 1)));
        y += 22;
    }

    y += 10;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.left(), y, "Direction Spread");
    y += 20;
    p.setFont(QFont("Sans", 9));

    QMap<QString, int> dirCounts;
    for (const auto& e : entries_) dirCounts[e.direction]++;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = dirCounts.begin(); it != dirCounts.end(); ++it) {
        QColor c = colors[ci++ % colors.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.left(), y, it.value() * 30, 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + it.value() * 30 + 6, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 20;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingVortex::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Diverging: %2 | Avg Velocity: %3")
        .arg(entries_.size())
        .arg(divergingCount())
        .arg(QString::number(avgVelocity(), 'f', 2)));
}

void PaperReadingVortex::loadSettings() {
    settings_.beginGroup("ReadingVortex");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        VortexEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.direction = settings_.value(QString("direction_%1").arg(i)).toString();
        e.velocity = settings_.value(QString("velocity_%1").arg(i)).toDouble();
        e.rotations = settings_.value(QString("rotations_%1").arg(i)).toInt();
        e.diverging = settings_.value(QString("diverging_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingVortex::saveSettings() {
    settings_.beginGroup("ReadingVortex");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("direction_%1").arg(i), e.direction);
        settings_.setValue(QString("velocity_%1").arg(i), e.velocity);
        settings_.setValue(QString("rotations_%1").arg(i), e.rotations);
        settings_.setValue(QString("diverging_%1").arg(i), e.diverging);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
