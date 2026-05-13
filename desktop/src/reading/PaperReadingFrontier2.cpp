#include "reading/PaperReadingFrontier2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingFrontier2::PaperReadingFrontier2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingFrontier2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QColor palette[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237)
        };
        QStringList territories = {"Quantum Realm", "Genome Jungle", "Neural Wilderness"};
        QStringList explorers = {"Researcher A", "Scientist B", "Scholar C"};
        QStringList categories = {"Physics", "Biology", "CS", "Chemistry", "Math"};
        struct Seed { QString territory; QString category; QString explorer; qreal coverage; int discoveries; bool mapped; };
        Seed seeds[] = {
            {"Quantum Realm",    "Physics",   "Researcher A", 0.82, 14, true},
            {"Quantum Realm",    "Chemistry", "Scientist B",  0.45,  7, false},
            {"Genome Jungle",    "Biology",   "Scholar C",    0.91, 22, true},
            {"Genome Jungle",    "CS",        "Researcher A", 0.38,  5, false},
            {"Neural Wilderness","CS",        "Scientist B",  0.73, 11, true},
            {"Neural Wilderness","Math",      "Scholar C",    0.55,  8, false},
            {"Quantum Realm",    "Math",      "Scholar C",    0.67,  9, true},
            {"Neural Wilderness","Biology",   "Researcher A", 0.60, 10, true},
        };
        for (int i = 0; i < 8; ++i) {
            ReadingFrontier2Entry e;
            e.id = i + 1;
            e.territory = seeds[i].territory;
            e.category = seeds[i].category;
            e.explorer = seeds[i].explorer;
            e.coverage = seeds[i].coverage;
            e.discoveries = seeds[i].discoveries;
            e.mapped = seeds[i].mapped;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperReadingFrontier2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    exploreBtn_ = new QPushButton("Explore");
    exploreBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(exploreBtn_, &QPushButton::clicked, this, &PaperReadingFrontier2::onExplore);
    toolbar->addWidget(exploreBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Physics", "Biology", "CS", "Chemistry", "Math"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter territory name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingFrontier2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Explore reading frontier");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 540);
}

void PaperReadingFrontier2::addEntry(const ReadingFrontier2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit territoryMapped(entry.id, entry.coverage);
    update();
}

QList<ReadingFrontier2Entry> PaperReadingFrontier2::entries() const {
    return entries_;
}

int PaperReadingFrontier2::mappedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.mapped) ++c;
    return c;
}

qreal PaperReadingFrontier2::avgCoverage() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.coverage;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingFrontier2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingFrontier2::onExplore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList territories = {"Quantum Realm", "Genome Jungle", "Neural Wilderness"};
    QStringList explorers = {"Researcher A", "Scientist B", "Scholar C"};
    QStringList categories = {"Physics", "Biology", "CS", "Chemistry", "Math"};
    QColor palette[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        ReadingFrontier2Entry e;
        e.id = entries_.size() + 1;
        e.territory = territories.contains(text) ? text
                      : territories[QRandomGenerator::global()->bounded(territories.size())];
        e.category = cIdx == 0
                     ? categories[QRandomGenerator::global()->bounded(categories.size())]
                     : categories[cIdx - 1];
        e.explorer = explorers[QRandomGenerator::global()->bounded(explorers.size())];
        e.coverage = (10 + QRandomGenerator::global()->bounded(90)) / 100.0;
        e.discoveries = 1 + QRandomGenerator::global()->bounded(25);
        e.mapped = QRandomGenerator::global()->bounded(100) < 55;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingFrontier2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Explore reading frontier");
    update();
}

void PaperReadingFrontier2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Explore reading frontier");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Frontier v2");

    int w = width(), h = height();
    drawFrontierView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingFrontier2::drawFrontierView(QPainter& p, const QRect& rect) {
    int show = qMin(8, entries_.size());
    int cardH = qMin(60, (rect.height() - 10) / qMax(show, 1));
    int cardW = rect.width();

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (cardH + 4);
        int x = rect.x();

        // Card background with map-like topographic shape
        QPainterPath cardShape;
        int wave = 6;
        cardShape.moveTo(x + 6, y);
        cardShape.lineTo(x + cardW - 6, y);
        // wavy right edge
        cardShape.cubicTo(x + cardW + 4, y + cardH * 0.25,
                          x + cardW - 4, y + cardH * 0.45,
                          x + cardW + wave, y + cardH * 0.5);
        cardShape.cubicTo(x + cardW - 4, y + cardH * 0.55,
                          x + cardW + 4, y + cardH * 0.75,
                          x + cardW - 6, y + cardH);
        cardShape.lineTo(x + 6, y + cardH);
        // wavy left edge
        cardShape.cubicTo(x - wave, y + cardH * 0.75,
                          x + 4, y + cardH * 0.55,
                          x - wave, y + cardH * 0.5);
        cardShape.cubicTo(x + 4, y + cardH * 0.45,
                          x - 4, y + cardH * 0.25,
                          x + 6, y);
        cardShape.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawPath(cardShape);

        // Territory contour lines (topographic map effect)
        p.setPen(QPen(e.color.lighter(140), 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        int inset = 8;
        QPainterPath contour;
        contour.addRoundedRect(x + inset, y + 4, cardW - 2 * inset, cardH - 8, 8, 8);
        p.drawPath(contour);

        // Coverage fill bar
        int barX = x + 10;
        int barY = y + cardH - 14;
        int barMaxW = cardW - 20;
        int barFillW = static_cast<int>(e.coverage * barMaxW);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barMaxW, 6, 3, 3);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, barY, barFillW, 6, 3, 3);

        // Territory name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(x + 12, y + 4, cardW - 24, 16, Qt::AlignVCenter,
                   e.territory);

        // Explorer badge
        QRectF badgeRect(x + cardW - 95, y + 4, 82, 16);
        p.setPen(Qt::NoPen);
        p.setBrush(e.mapped ? QColor(22, 163, 74, 40) : QColor(220, 38, 38, 40));
        p.drawRoundedRect(badgeRect, 8, 8);
        p.setPen(e.mapped ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.setFont(QFont("Arial", 7));
        p.drawText(badgeRect, Qt::AlignCenter, e.explorer);

        // Category + discovery count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QString detail = e.category
                         + "  |  " + QString::number(e.discoveries) + " discoveries"
                         + "  |  " + QString::number(static_cast<int>(e.coverage * 100)) + "%";
        p.drawText(x + 12, y + 20, cardW - 24, 14, Qt::AlignVCenter, detail);

        // Discovery markers (dots)
        int dotCount = qMin(e.discoveries, 12);
        int dotY = y + cardH - 24;
        for (int d = 0; d < dotCount; ++d) {
            int dotX = x + 14 + d * 9;
            if (dotX > x + cardW - 14) break;
            p.setPen(Qt::NoPen);
            p.setBrush(e.color);
            p.drawEllipse(dotX, dotY, 5, 5);
        }

        // Mapped flag
        if (e.mapped) {
            p.setPen(QPen(QColor(22, 163, 74), 1.5));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(x + cardW - 48, y + cardH - 22, 36, 12, Qt::AlignCenter, "MAPPED");
        } else {
            p.setPen(QPen(QColor(220, 38, 38), 1.5));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(x + cardW - 58, y + cardH - 22, 46, 12, Qt::AlignCenter, "UNMAPPED");
        }
    }
}

void PaperReadingFrontier2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    QStringList allCats = {"Physics", "Biology", "CS", "Chemistry", "Math"};
    QColor palette[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int total = 0;
    for (const auto& cat : allCats) total += counts.value(cat, 0);
    if (total == 0) total = 1;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + (rect.height() - 40) / 2;
    int outerR = qMin(rect.width(), rect.height() - 40) / 2 - 10;
    int innerR = outerR * 55 / 100;

    qreal startAngle = 0.0;
    for (int i = 0; i < allCats.size(); ++i) {
        int count = counts.value(allCats[i], 0);
        if (count == 0) continue;
        qreal span = (static_cast<qreal>(count) / total) * 360.0;

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));

        // Label
        qreal midAngle = qDegreesToRadians(startAngle + span / 2.0);
        int labelR = outerR + 16;
        int lx = cx + static_cast<int>(labelR * qCos(midAngle));
        int ly = cy - static_cast<int>(labelR * qSin(midAngle));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(lx - 24, ly - 6, 48, 12, Qt::AlignCenter,
                   allCats[i] + " (" + QString::number(count) + ")");

        startAngle += span;
    }

    // Inner circle (donut hole)
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(cx - innerR, cy - 12, innerR * 2, 24, Qt::AlignCenter,
               QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    p.drawText(cx - innerR, cy + 6, innerR * 2, 16, Qt::AlignCenter,
               "total entries");
}

void PaperReadingFrontier2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Territories", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Mapped Count",      QString::number(mappedCount()),  QColor(22, 163, 74)},
        {"Avg Coverage",      QString::number(avgCoverage() * 100, 'f', 1) + "%", QColor(217, 119, 6)},
        {"Total Discoveries", QString::number(std::accumulate(entries_.constBegin(),
                                                              entries_.constEnd(), 0,
                                                              [](int s, const ReadingFrontier2Entry& e) {
                                                                  return s + e.discoveries;
                                                              })),
         QColor(124, 58, 237)}
    };

    int cols = 2;
    int rows = 2;
    int gapX = 8;
    int gapY = 8;
    int boxW = (rect.width() - (cols - 1) * gapX) / cols;
    int boxH = (rect.height() - (rows - 1) * gapY) / rows;

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int bx = rect.x() + col * (boxW + gapX);
        int by = rect.y() + row * (boxH + gapY);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, boxW, boxH, 8, 8);

        // Left accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, by, 5, boxH, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(bx + 12, by + 6, boxW - 20, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(bx + 12, by + boxH / 2, boxW - 20, boxH / 2 - 4, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperReadingFrontier2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Explore reading frontier");
        return;
    }
    int totalDisc = 0;
    for (const auto& e : entries_) totalDisc += e.discoveries;
    infoLabel_->setText(QString("%1 territories | %2 mapped | avg %3% coverage | %4 discoveries")
        .arg(entries_.size())
        .arg(mappedCount())
        .arg(static_cast<int>(avgCoverage() * 100))
        .arg(totalDisc));
}

void PaperReadingFrontier2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingFrontier2Entry e;
        e.id = settings_.value("id").toInt();
        e.territory = settings_.value("territory").toString();
        e.category = settings_.value("category").toString();
        e.explorer = settings_.value("explorer").toString();
        e.coverage = settings_.value("coverage").toDouble();
        e.discoveries = settings_.value("discoveries").toInt();
        e.mapped = settings_.value("mapped").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingFrontier2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("territory", entries_[i].territory);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("explorer", entries_[i].explorer);
        settings_.setValue("coverage", entries_[i].coverage);
        settings_.setValue("discoveries", entries_[i].discoveries);
        settings_.setValue("mapped", entries_[i].mapped);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
