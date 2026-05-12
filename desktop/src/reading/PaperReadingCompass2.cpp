#include "reading/PaperReadingCompass2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingCompass2::PaperReadingCompass2(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingCompass2") {
    setupUI();
    loadSettings();
}

void PaperReadingCompass2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"Theory", "Application", "Survey", "Experimental", "Review"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Paper title...");

    trackBtn_ = new QPushButton("Track", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel("Readings: 0 | Focused: 0 | Avg Score: 0.00", this);

    leftLayout->addWidget(categoryCombo_);
    leftLayout->addWidget(inputField_);
    leftLayout->addWidget(trackBtn_);
    leftLayout->addWidget(clearBtn_);
    leftLayout->addWidget(infoLabel_);
    leftLayout->addStretch();

    mainLayout->addWidget(leftPanel);
    mainLayout->addStretch();

    connect(trackBtn_, &QPushButton::clicked, this, &PaperReadingCompass2::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingCompass2::onClear);
}

void PaperReadingCompass2::onTrack() {
    CompassEntry e;
    e.id = entries_.size() + 1;
    e.paper = inputField_->text().trimmed();
    if (e.paper.isEmpty()) e.paper = QString("Paper_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.score = QRandomGenerator::global()->generateDouble();
    e.pagesRead = QRandomGenerator::global()->bounded(1, 101);
    e.focused = e.score > 0.7;
    QStringList dirs = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    e.direction = dirs[QRandomGenerator::global()->bounded(dirs.size())];
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    emit directionSet(e.id, e.score);
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingCompass2::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingCompass2::addEntry(const CompassEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<CompassEntry> PaperReadingCompass2::entries() const {
    return entries_;
}

int PaperReadingCompass2::focusedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.focused) c++;
    return c;
}

qreal PaperReadingCompass2::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingCompass2::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingCompass2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    int colW = w / 3;
    drawCompassView(p, QRect(10, 10, colW - 20, h - 20));
    drawCategoryChart(p, QRect(colW + 10, 10, colW - 20, h - 20));
    drawStats(p, QRect(2 * colW + 10, 10, colW - 20, h - 20));
}

void PaperReadingCompass2::drawCompassView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.adjusted(0, 0, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Reading Compass");

    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int radius = qMin(rect.width(), rect.height()) / 2 - 40;

    // Draw compass rings
    for (int r = 1; r <= 3; ++r) {
        int ringR = radius * r / 3;
        p.setPen(QPen(QColor(0xe2e8f0), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cx - ringR, cy - ringR, ringR * 2, ringR * 2);
    }

    // Draw axis lines
    p.setPen(QPen(QColor(0x94a3b8), 1));
    p.drawLine(cx - radius, cy, cx + radius, cy);
    p.drawLine(cx, cy - radius, cx, cy + radius);

    // Direction labels and indicators
    QStringList dirs = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    qreal dirAngles[8] = {-M_PI / 2, -M_PI / 4, 0, M_PI / 4, M_PI / 2, 3 * M_PI / 4, M_PI, -3 * M_PI / 4};

    // Count entries per direction to determine brightness
    QMap<QString, int> dirFocusedCount;
    for (const auto& e : entries_) {
        if (e.focused) dirFocusedCount[e.direction]++;
    }

    p.setFont(QFont("Sans", 9));
    for (int i = 0; i < 8; ++i) {
        int focused = dirFocusedCount.value(dirs[i], 0);
        bool hasFocused = focused > 0;

        // Draw direction indicator tick
        int innerR = radius - 8;
        int outerR = radius + 2;
        if (hasFocused) {
            p.setPen(QPen(QColor(0x3b82f6), 3));
        } else {
            p.setPen(QPen(QColor(0xcbd5e1), 1));
        }
        int x1 = cx + static_cast<int>(innerR * qCos(dirAngles[i]));
        int y1 = cy + static_cast<int>(innerR * qSin(dirAngles[i]));
        int x2 = cx + static_cast<int>(outerR * qCos(dirAngles[i]));
        int y2 = cy + static_cast<int>(outerR * qSin(dirAngles[i]));
        p.drawLine(x1, y1, x2, y2);

        // Draw direction label
        if (hasFocused) {
            p.setPen(QColor(0x3b82f6));
        } else {
            p.setPen(QColor(0x94a3b8));
        }
        int labelR = radius + 16;
        int lx = cx + static_cast<int>(labelR * qCos(dirAngles[i])) - 6;
        int ly = cy + static_cast<int>(labelR * qSin(dirAngles[i])) + 4;
        p.drawText(lx, ly, dirs[i]);
    }

    // Draw entry dots on compass
    for (const auto& e : entries_) {
        int idx = dirs.indexOf(e.direction);
        if (idx < 0) idx = 0;
        qreal angle = dirAngles[idx];
        qreal r = e.score * radius * 0.85;
        int px = cx + static_cast<int>(r * qCos(angle));
        int py = cy + static_cast<int>(r * qSin(angle));

        if (e.focused) {
            p.setPen(Qt::NoPen);
            p.setBrush(e.color);
            p.drawEllipse(px - 5, py - 5, 10, 10);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(e.color.red(), e.color.green(), e.color.blue(), 80));
            p.drawEllipse(px - 4, py - 4, 8, 8);
        }
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingCompass2::drawCategoryChart(QPainter& p, const QRect& rect) {
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

        // Category label
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.left(), y + 12, it.key());

        // Horizontal bar
        int barW = maxCount > 0 ? (it.value() * barMaxW / maxCount) : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.left() + 80, y, barW, 16, 3, 3);

        // Count label
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 80 + barW + 6, y + 13, QString::number(it.value()));

        y += 24;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingCompass2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int y = rect.top() + 28;
    p.setFont(QFont("Sans", 9));

    // Total readings
    p.drawText(rect.left(), y, QString("Total Readings: %1").arg(entries_.size()));
    y += 22;

    // Focused count
    p.drawText(rect.left(), y, QString("Focused: %1").arg(focusedCount()));
    y += 22;

    // Average score
    p.drawText(rect.left(), y, QString("Avg Score: %1").arg(QString::number(avgScore(), 'f', 2)));
    y += 22;

    // Average pages read
    if (!entries_.isEmpty()) {
        int totalPages = 0;
        for (const auto& e : entries_) totalPages += e.pagesRead;
        p.drawText(rect.left(), y, QString("Avg Pages: %1").arg(QString::number(static_cast<qreal>(totalPages) / entries_.size(), 'f', 1)));
        y += 22;
    }

    // Direction distribution
    y += 10;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.left(), y, "Direction Distribution");
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

void PaperReadingCompass2::updateInfo() {
    infoLabel_->setText(QString("Readings: %1 | Focused: %2 | Avg Score: %3")
        .arg(entries_.size())
        .arg(focusedCount())
        .arg(QString::number(avgScore(), 'f', 2)));
}

void PaperReadingCompass2::loadSettings() {
    settings_.beginGroup("ReadingCompass2");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        CompassEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.direction = settings_.value(QString("direction_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.score = settings_.value(QString("score_%1").arg(i)).toDouble();
        e.pagesRead = settings_.value(QString("pagesRead_%1").arg(i)).toInt();
        e.focused = settings_.value(QString("focused_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingCompass2::saveSettings() {
    settings_.beginGroup("ReadingCompass2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("direction_%1").arg(i), e.direction);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("score_%1").arg(i), e.score);
        settings_.setValue(QString("pagesRead_%1").arg(i), e.pagesRead);
        settings_.setValue(QString("focused_%1").arg(i), e.focused);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
