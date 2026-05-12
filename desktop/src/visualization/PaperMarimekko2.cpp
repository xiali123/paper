#include "visualization/PaperMarimekko2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperMarimekko2::PaperMarimekko2(QWidget* parent)
    : QWidget(parent), settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperMarimekko2") {
    setupUI();
    loadSettings();
}

void PaperMarimekko2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Toolbar
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Market", "Revenue", "Volume", "Growth"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Segment:w,h");
    renderBtn_ = new QPushButton("Render", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Entries: 0 | Highlighted: 0 | Area: 0.0", this);

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    mainLayout->addStretch();

    connect(renderBtn_, &QPushButton::clicked, this, &PaperMarimekko2::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMarimekko2::onClear);
}

void PaperMarimekko2::addEntry(const Marimekko2Entry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<Marimekko2Entry> PaperMarimekko2::entries() const { return entries_; }

int PaperMarimekko2::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlighted) c++;
    return c;
}

qreal PaperMarimekko2::totalArea() const {
    qreal total = 0;
    for (const auto& e : entries_) total += e.area;
    return total;
}

QMap<QString, int> PaperMarimekko2::categoryCounts() const {
    QMap<QString, int> map;
    for (const auto& e : entries_) map[e.category]++;
    return map;
}

void PaperMarimekko2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();

    // Background
    p.fillRect(rect(), QColor(0xf8fafc));

    // Layout: top half for chart, bottom split into legend (left) and stats (right)
    int toolbarH = 50;
    int chartTop = toolbarH;
    int chartH = (h - toolbarH) / 2;
    int bottomTop = chartTop + chartH;
    int bottomH = h - bottomTop;

    drawMarimekko(p, QRect(10, chartTop, w - 20, chartH - 5));
    drawCategoryLegend(p, QRect(10, bottomTop, w / 2 - 15, bottomH - 10));
    drawStats(p, QRect(w / 2 + 5, bottomTop, w / 2 - 15, bottomH - 10));
}

void PaperMarimekko2::drawMarimekko(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Marimekko Chart");

    if (entries_.isEmpty()) return;

    // Category color map
    QMap<QString, QColor> catColors;
    catColors["Market"] = QColor(0x3b82f6);
    catColors["Revenue"] = QColor(0x16a34a);
    catColors["Volume"] = QColor(0x7c3aed);
    catColors["Growth"] = QColor(0xd97706);

    // Compute total width across all entries for proportional sizing
    qreal totalWidth = 0;
    for (const auto& e : entries_) totalWidth += e.width;
    if (totalWidth <= 0) totalWidth = 1.0;

    // Build column structure: group entries by segment (axis)
    // Each segment becomes a column; entries within a column stack vertically
    QMap<QString, QList<int>> segmentCols;
    QList<QString> segmentOrder;
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        if (!segmentCols.contains(e.segment)) {
            segmentOrder.append(e.segment);
        }
        segmentCols[e.segment].append(i);
    }

    // Compute total height per column for normalization
    QMap<QString, qreal> colTotalHeight;
    for (const auto& seg : segmentOrder) {
        qreal colH = 0;
        for (int idx : segmentCols[seg]) colH += entries_[idx].height;
        colTotalHeight[seg] = colH;
    }

    // Overall total height (max column height) for vertical normalization
    qreal maxColHeight = 0;
    for (const auto& seg : segmentOrder) {
        if (colTotalHeight[seg] > maxColHeight) maxColHeight = colTotalHeight[seg];
    }
    if (maxColHeight <= 0) maxColHeight = 1.0;

    int drawLeft = rect.left() + 5;
    int drawTop = rect.top() + 28;
    int drawW = rect.width() - 10;
    int drawH = rect.height() - 38;

    // Compute total segment width for proportional column widths
    QMap<QString, qreal> segWidthSum;
    qreal allSegWidthSum = 0;
    for (const auto& seg : segmentOrder) {
        qreal sw = 0;
        for (int idx : segmentCols[seg]) sw += entries_[idx].width;
        segWidthSum[seg] = sw;
        allSegWidthSum += sw;
    }
    if (allSegWidthSum <= 0) allSegWidthSum = 1.0;

    int x = drawLeft;

    for (const auto& seg : segmentOrder) {
        int colW = static_cast<int>(segWidthSum[seg] / allSegWidthSum * drawW);
        colW = qMax(colW, 4);
        if (x + colW > drawLeft + drawW) colW = drawLeft + drawW - x;
        if (colW <= 0) break;

        int y = drawTop + drawH; // bottom of chart area, stack upward

        for (int idx : segmentCols[seg]) {
            const auto& e = entries_[idx];
            // Height proportional within this column
            qreal hRatio = e.height / colTotalHeight[seg];
            int cellH = static_cast<int>(hRatio * drawH);
            cellH = qMax(cellH, 3);

            int cellTop = y - cellH;
            if (cellTop < drawTop) cellTop = drawTop;
            int actualH = y - cellTop;
            if (actualH <= 0) { y = cellTop; continue; }

            // Color by category
            QColor fillColor = catColors.value(e.category, QColor(0x94a3b8));
            fillColor.setAlpha(204);

            QPainterPath cellPath;
            cellPath.addRoundedRect(x + 1, cellTop, colW - 2, actualH, 3.0, 3.0);
            p.setBrush(fillColor);
            p.setPen(QColor(0xffffff));
            p.drawPath(cellPath);

            // Highlighted entries get bold border
            if (e.highlighted) {
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(QColor(0x0f172a), 2.5));
                QPainterPath borderPath;
                borderPath.addRoundedRect(x + 1, cellTop, colW - 2, actualH, 3.0, 3.0);
                p.drawPath(borderPath);
            }

            // Labels inside cells
            if (colW > 40 && actualH > 22) {
                p.setPen(QColor(0xffffff));
                p.setFont(QFont("Sans", 8, QFont::Bold));
                QString label = e.segment;
                if (actualH > 36) {
                    label += QString("\n%1").arg(e.area);
                }
                p.drawText(QRect(x + 3, cellTop + 2, colW - 6, actualH - 4),
                           Qt::AlignCenter, label);
            }

            y = cellTop;
        }

        // Segment label at bottom
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        if (colW > 30) {
            p.drawText(QRect(x, drawTop + drawH + 1, colW, 16),
                       Qt::AlignCenter, seg);
        }

        x += colW;
    }
}

void PaperMarimekko2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Legend");

    QMap<QString, QColor> catColors;
    catColors["Market"] = QColor(0x3b82f6);
    catColors["Revenue"] = QColor(0x16a34a);
    catColors["Volume"] = QColor(0x7c3aed);
    catColors["Growth"] = QColor(0xd97706);

    auto counts = categoryCounts();
    int y = rect.top() + 25;

    for (auto it = catColors.begin(); it != catColors.end(); ++it) {
        if (y + 22 > rect.bottom()) break;
        int count = counts.value(it.key(), 0);

        p.setBrush(it.value());
        p.setPen(Qt::NoPen);
        QPainterPath box;
        box.addRoundedRect(rect.left(), y, 14, 14, 2.0, 2.0);
        p.drawPath(box);

        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.left() + 20, y + 12,
                   QString("%1: %2").arg(it.key()).arg(count));
        y += 22;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperMarimekko2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int y = rect.top() + 25;
    p.setFont(QFont("Sans", 9));

    qreal avgSize = entries_.isEmpty() ? 0.0 : totalArea() / entries_.size();

    p.drawText(rect.left(), y, QString("Total area: %1").arg(totalArea()));
    y += 18;
    p.drawText(rect.left(), y, QString("Avg size: %1").arg(QString::number(avgSize, 'f', 1)));
    y += 18;
    p.drawText(rect.left(), y, QString("Highlighted: %1").arg(highlightedCount()));
    y += 18;
    p.drawText(rect.left(), y, QString("Entries: %1").arg(entries_.size()));
}

void PaperMarimekko2::onRender() {
    Marimekko2Entry e;
    e.id = entries_.size() + 1;

    // Parse "Segment:w,h" input
    QString text = inputField_->text().trimmed();
    QStringList parts = text.split(':');
    if (parts.size() >= 2) {
        e.segment = parts[0].isEmpty() ? QString("Seg_%1").arg(e.id) : parts[0];
        QStringList dims = parts[1].split(',');
        e.width = dims.size() >= 1 ? dims[0].toDouble() : 0.0;
        e.height = dims.size() >= 2 ? dims[1].toDouble() : 0.0;
    } else {
        e.segment = text.isEmpty() ? QString("Seg_%1").arg(e.id) : text;
        e.width = 0.2 + QRandomGenerator::global()->bounded(0.8);
        e.height = 0.2 + QRandomGenerator::global()->bounded(0.8);
    }
    if (e.width <= 0) e.width = 0.2 + QRandomGenerator::global()->bounded(0.8);
    if (e.height <= 0) e.height = 0.2 + QRandomGenerator::global()->bounded(0.8);

    // Category from combo
    QString cat = categoryCombo_->currentText();
    if (cat == "All") {
        QList<QString> cats = {"Market", "Revenue", "Volume", "Growth"};
        cat = cats[QRandomGenerator::global()->bounded(cats.size())];
    }
    e.category = cat;

    e.axis = e.segment;
    e.area = static_cast<int>(e.width * e.height * 100.0);
    e.highlighted = e.area > 50;

    // Color by category
    QMap<QString, QColor> catColors;
    catColors["Market"] = QColor(0x3b82f6);
    catColors["Revenue"] = QColor(0x16a34a);
    catColors["Volume"] = QColor(0x7c3aed);
    catColors["Growth"] = QColor(0xd97706);
    e.color = catColors.value(e.category, QColor(0x94a3b8));

    entries_.append(e);
    updateInfo();
    saveSettings();
    emit segmentClicked(e.id, static_cast<qreal>(e.area));
    update();
}

void PaperMarimekko2::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperMarimekko2::updateInfo() {
    infoLabel_->setText(QString("Entries: %1 | Highlighted: %2 | Area: %3")
        .arg(entries_.size())
        .arg(highlightedCount())
        .arg(QString::number(totalArea(), 'f', 1)));
}

void PaperMarimekko2::loadSettings() {
    settings_.beginGroup("Marimekko2");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        Marimekko2Entry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.segment = settings_.value(QString("segment_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.axis = settings_.value(QString("axis_%1").arg(i)).toString();
        e.width = settings_.value(QString("width_%1").arg(i)).toDouble();
        e.height = settings_.value(QString("height_%1").arg(i)).toDouble();
        e.area = settings_.value(QString("area_%1").arg(i)).toInt();
        e.highlighted = settings_.value(QString("highlighted_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperMarimekko2::saveSettings() {
    settings_.beginGroup("Marimekko2");
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
        settings_.setValue(QString("highlighted_%1").arg(i), e.highlighted);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
