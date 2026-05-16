#include "reading/PaperReadingLabyrinth.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingLabyrinth::PaperReadingLabyrinth(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingLabyrinth") {
    setupUI();
    loadSettings();
}

void PaperReadingLabyrinth::setupUI() {
    auto* toolbar = new QHBoxLayout();

    navigateBtn_ = new QPushButton("Navigate", this);
    navigateBtn_->setStyleSheet("QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;padding:6px 16px;font-weight:bold;}");

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"Theory", "Application", "Survey", "Experimental", "Review"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Section name...");

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet("QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;padding:6px 16px;font-weight:bold;}");

    toolbar->addWidget(navigateBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();

    infoLabel_ = new QLabel("Entries: 0 | Solved: 0 | Avg Complexity: 0.00", this);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    mainLayout->addStretch();

    connect(navigateBtn_, &QPushButton::clicked, this, &PaperReadingLabyrinth::onNavigate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingLabyrinth::onClear);
}

void PaperReadingLabyrinth::onNavigate() {
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList categories = {"Theory", "Application", "Survey", "Experimental", "Review"};
    QStringList directions = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

    int count = QRandomGenerator::global()->bounded(3, 7);
    for (int i = 0; i < count; ++i) {
        LabyrinthEntry e;
        e.id = entries_.size() + 1;
        e.section = inputField_->text().trimmed();
        if (e.section.isEmpty()) e.section = QString("Section_%1").arg(e.id);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        QStringList pathParts;
        int pathLen = QRandomGenerator::global()->bounded(2, 5);
        for (int j = 0; j < pathLen; ++j)
            pathParts.append(directions[QRandomGenerator::global()->bounded(directions.size())]);
        e.path = pathParts.join("->");
        e.complexity = QRandomGenerator::global()->generateDouble() * 4.0 + 0.5;
        e.turns = QRandomGenerator::global()->bounded(1, 10);
        e.solved = QRandomGenerator::global()->generateDouble() > 0.5;
        e.color = colors[e.id % colors.size()];
        entries_.append(e);
        if (e.solved) emit mazeSolved(e.id, e.complexity);
    }
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingLabyrinth::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingLabyrinth::addEntry(const LabyrinthEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<LabyrinthEntry> PaperReadingLabyrinth::entries() const {
    return entries_;
}

int PaperReadingLabyrinth::solvedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.solved) c++;
    return c;
}

qreal PaperReadingLabyrinth::avgComplexity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.complexity;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingLabyrinth::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingLabyrinth::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    int colW = w / 3;
    drawLabyrinthMap(p, QRect(10, 10, colW - 20, h - 20));
    drawCategoryChart(p, QRect(colW + 10, 10, colW - 20, h - 20));
    drawStats(p, QRect(2 * colW + 10, 10, colW - 20, h - 20));
}

void PaperReadingLabyrinth::drawLabyrinthMap(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Reading Labyrinth");

    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + rect.height() / 2 + 10;
    int radius = qMin(rect.width(), rect.height()) / 2 - 40;

    p.setPen(QPen(QColor(0xe2e8f0), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect.left() + 20, rect.top() + 24, rect.width() - 40, rect.height() - 44);

    int gridSize = 6;
    int cellW = (rect.width() - 40) / gridSize;
    int cellH = (rect.height() - 44) / gridSize;
    for (int i = 1; i < gridSize; ++i) {
        int x = rect.left() + 20 + i * cellW;
        int y = rect.top() + 24 + i * cellH;
        p.drawLine(x, rect.top() + 24, x, rect.top() + 24 + gridSize * cellH);
        p.drawLine(rect.left() + 20, y, rect.left() + 20 + gridSize * cellW, y);
    }

    for (const auto& e : entries_) {
        qreal angle = (e.id * 137.508 + e.turns * 45.0) * M_PI / 180.0;
        qreal r = e.complexity / 4.5 * radius * 0.85;
        int px = cx + static_cast<int>(r * qCos(angle));
        int py = cy + static_cast<int>(r * qSin(angle));

        int dotSize = qBound(5, e.turns + 2, 14);

        p.setPen(QPen(QColor(e.color.red(), e.color.green(), e.color.blue(), 100), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(px - dotSize, py - dotSize, dotSize * 2, dotSize * 2);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        if (e.solved) {
            p.drawEllipse(px - dotSize / 2, py - dotSize / 2, dotSize, dotSize);
        } else {
            QPolygonF diamond;
            diamond << QPointF(px, py - dotSize / 2)
                    << QPointF(px + dotSize / 2, py)
                    << QPointF(px, py + dotSize / 2)
                    << QPointF(px - dotSize / 2, py);
            p.drawPolygon(diamond);
        }

        qreal nextAngle = angle + 0.25;
        int nx = cx + static_cast<int>((r + 10) * qCos(nextAngle));
        int ny = cy + static_cast<int>((r + 10) * qSin(nextAngle));
        p.setPen(QPen(QColor(e.color.red(), e.color.green(), e.color.blue(), 80), 1));
        p.drawLine(px, py, nx, ny);
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingLabyrinth::drawCategoryChart(QPainter& p, const QRect& rect) {
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

void PaperReadingLabyrinth::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int y = rect.top() + 28;
    p.setFont(QFont("Sans", 9));

    p.drawText(rect.left(), y, QString("Total Entries: %1").arg(entries_.size()));
    y += 22;

    p.drawText(rect.left(), y, QString("Solved: %1").arg(solvedCount()));
    y += 22;

    p.drawText(rect.left(), y, QString("Avg Complexity: %1").arg(QString::number(avgComplexity(), 'f', 2)));
    y += 22;

    if (!entries_.isEmpty()) {
        int totalTurns = 0;
        for (const auto& e : entries_) totalTurns += e.turns;
        p.drawText(rect.left(), y, QString("Avg Turns: %1").arg(QString::number(static_cast<qreal>(totalTurns) / entries_.size(), 'f', 1)));
        y += 22;
    }

    y += 10;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.left(), y, "Solve Rate");
    y += 20;
    p.setFont(QFont("Sans", 9));

    if (!entries_.isEmpty()) {
        qreal rate = static_cast<qreal>(solvedCount()) / entries_.size();
        int barW = static_cast<int>(rate * (rect.width() - 20));
        QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
        p.setPen(Qt::NoPen);
        p.setBrush(colors[0]);
        p.drawRoundedRect(rect.left(), y, barW, 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + barW + 6, y + 12, QString("%1%").arg(QString::number(rate * 100, 'f', 0)));
        y += 24;
    }

    y += 10;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.left(), y, "Turn Distribution");
    y += 20;
    p.setFont(QFont("Sans", 9));

    QMap<int, int> turnCounts;
    for (const auto& e : entries_) turnCounts[e.turns]++;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = turnCounts.begin(); it != turnCounts.end(); ++it) {
        QColor c = colors[ci++ % colors.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.left(), y, it.value() * 30, 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + it.value() * 30 + 6, y + 12, QString("%1 turns: %2").arg(it.key()).arg(it.value()));
        y += 20;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingLabyrinth::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Solved: %2 | Avg Complexity: %3")
        .arg(entries_.size())
        .arg(solvedCount())
        .arg(QString::number(avgComplexity(), 'f', 2)));
}

void PaperReadingLabyrinth::loadSettings() {
    settings_.beginGroup("ReadingLabyrinth");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        LabyrinthEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.section = settings_.value(QString("section_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.path = settings_.value(QString("path_%1").arg(i)).toString();
        e.complexity = settings_.value(QString("complexity_%1").arg(i)).toDouble();
        e.turns = settings_.value(QString("turns_%1").arg(i)).toInt();
        e.solved = settings_.value(QString("solved_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingLabyrinth::saveSettings() {
    settings_.beginGroup("ReadingLabyrinth");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("section_%1").arg(i), e.section);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("path_%1").arg(i), e.path);
        settings_.setValue(QString("complexity_%1").arg(i), e.complexity);
        settings_.setValue(QString("turns_%1").arg(i), e.turns);
        settings_.setValue(QString("solved_%1").arg(i), e.solved);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
