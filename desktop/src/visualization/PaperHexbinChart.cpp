#include "visualization/PaperHexbinChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperHexbinChart::PaperHexbinChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HexbinChart")
{
    setupUI();
    loadSettings();
}

void PaperHexbinChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Distribution", "Cluster", "Density", "Scatter"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("x,y coordinates...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperHexbinChart::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHexbinChart::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render hexbin chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperHexbinChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render hexbin chart");
        return;
    }

    int w = width(), h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Hexbin Chart");

    // Top half: hexbin grid
    drawHexbin(p, QRect(20, 50, w - 40, h / 2 - 50));

    // Bottom-left quarter: category legend
    drawCategoryLegend(p, QRect(20, h / 2 + 10, w / 2 - 30, h / 2 - 30));

    // Bottom-right quarter: stats
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 30, h / 2 - 30));
}

void PaperHexbinChart::drawHexbin(QPainter& p, const QRect& rect) {
    static const qreal hexSize = 20.0;
    static const qreal sqrt3 = 1.7320508075688772;

    // Build a density map keyed by hex grid coordinates
    struct HexCell {
        int count = 0;
        bool hasOutlier = false;
        QColor representativeColor;
        qreal worldX = 0;
        qreal worldY = 0;
    };

    if (entries_.isEmpty()) return;

    // Determine data bounds
    qreal minX = entries_[0].x, maxX = entries_[0].x;
    qreal minY = entries_[0].y, maxY = entries_[0].y;
    for (const auto& e : entries_) {
        if (e.x < minX) minX = e.x;
        if (e.x > maxX) maxX = e.x;
        if (e.y < minY) minY = e.y;
        if (e.y > maxY) maxY = e.y;
    }
    qreal rangeX = (maxX - minX) > 0 ? (maxX - minX) : 1.0;
    qreal rangeY = (maxY - minY) > 0 ? (maxY - minY) : 1.0;

    // Map data to hex grid and accumulate density per cell
    QMap<QPair<int,int>, HexCell> cells;
    int maxDensity = 1;
    for (const auto& e : entries_) {
        // Normalize to [0..1]
        qreal nx = (e.x - minX) / rangeX;
        qreal ny = (e.y - minY) / rangeY;

        // Axial hex coordinates (pointy-top orientation)
        qreal q = (2.0 / 3.0 * nx) / (hexSize / 200.0);
        qreal r = (-1.0 / 3.0 * nx + sqrt3 / 3.0 * ny) / (hexSize / 200.0);
        int col = qFloor(q);
        int row = qFloor(r);
        auto key = qMakePair(col, row);
        auto& cell = cells[key];
        cell.count++;
        cell.worldX += e.x;
        cell.worldY += e.y;
        cell.representativeColor = e.color;
        if (e.outlier) cell.hasOutlier = true;
        if (cell.count > maxDensity) maxDensity = cell.count;
    }

    // Average world positions for each cell
    for (auto it = cells.begin(); it != cells.end(); ++it) {
        it->worldX /= it->count;
        it->worldY /= it->count;
    }

    int margin = 15;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;

    // Draw axes
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.x() + margin, rect.y() + margin,
               rect.x() + margin, rect.y() + margin + plotH);
    p.drawLine(rect.x() + margin, rect.y() + margin + plotH,
               rect.x() + margin + plotW, rect.y() + margin + plotH);

    // Draw each hexagonal cell
    for (auto it = cells.constBegin(); it != cells.constEnd(); ++it) {
        const auto& cell = it.value();
        qreal nx = (cell.worldX - minX) / rangeX;
        qreal ny = (cell.worldY - minY) / rangeY;

        qreal cx = rect.x() + margin + nx * plotW;
        qreal cy = rect.y() + margin + plotH - ny * plotH;

        // Density fraction [0..1]
        qreal frac = static_cast<qreal>(cell.count) / static_cast<qreal>(maxDensity);

        // Color gradient: light (high alpha blending with white) to dark (full color)
        QColor baseColor = cell.representativeColor;
        int lightness = 240 - static_cast<int>(180 * frac);
        QColor fill = baseColor.lighter(static_cast<int>(100 + 200 * (1.0 - frac)));
        fill.setAlpha(80 + static_cast<int>(175 * frac));

        // Build hexagon path (pointy-top)
        QPainterPath hexPath;
        for (int i = 0; i < 6; ++i) {
            qreal angle = M_PI / 180.0 * (60.0 * i - 30.0);
            qreal hx = cx + hexSize * qCos(angle);
            qreal hy = cy + hexSize * qSin(angle);
            if (i == 0) hexPath.moveTo(hx, hy);
            else hexPath.lineTo(hx, hy);
        }
        hexPath.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(hexPath);

        // Outlier border
        if (cell.hasOutlier) {
            p.setPen(QPen(QColor(239, 68, 68), 2.5));
            p.setBrush(Qt::NoBrush);
            p.drawPath(hexPath);
        }

        // Light outline for all cells
        p.setPen(QPen(QColor(203, 213, 225, 120), 0.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(hexPath);

        // Density label inside hex
        if (cell.count > 1) {
            p.setPen(QColor(15, 23, 42, 180));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(QRectF(cx - 8, cy - 6, 16, 12), Qt::AlignCenter,
                       QString::number(cell.count));
        }
    }
}

void PaperHexbinChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    struct CatInfo { QString key; QString label; QColor color; };
    CatInfo categories[] = {
        {"Distribution", "Distribution", QColor(59, 130, 246)},
        {"Cluster",     "Cluster",     QColor(22, 163, 74)},
        {"Density",     "Density",     QColor(124, 58, 237)},
        {"Scatter",     "Scatter",     QColor(217, 119, 6)}
    };

    int itemH = qMin(28, (rect.height() - 50) / 5);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i].key) ? counts[categories[i].key] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(categories[i].color);
        p.drawRoundedRect(rect.x() + 5, y + 2, 14, 14, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i].label);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " pts");
    }

    // Outlier row
    int y = rect.y() + 22 + 4 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(239, 68, 68));
    p.drawRoundedRect(rect.x() + 5, y + 2, 14, 14, 3, 3);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Outliers");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(outlierCount()));
}

void PaperHexbinChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Points", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Density",  QString::number(avgDensity(), 'f', 1), QColor(124, 58, 237)},
        {"Outliers",     QString::number(outlierCount()), QColor(239, 68, 68)},
        {"Categories",   QString::number(categoryCounts().size()), QColor(217, 119, 6)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperHexbinChart::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList parts = text.split(',');
    if (parts.size() < 2) return;

    bool okX = false, okY = false;
    qreal xVal = parts[0].trimmed().toDouble(&okX);
    qreal yVal = parts[1].trimmed().toDouble(&okY);
    if (!okX || !okY) return;

    QString category;
    QColor color;
    int idx = categoryCombo_->currentIndex();
    switch (idx) {
        case 1: category = "Distribution"; color = QColor(59, 130, 246);  break;
        case 2: category = "Cluster";     color = QColor(22, 163, 74);   break;
        case 3: category = "Density";     color = QColor(124, 58, 237);  break;
        case 4: category = "Scatter";     color = QColor(217, 119, 6);   break;
        default: category = "Distribution"; color = QColor(59, 130, 246); break;
    }

    HexbinEntry entry;
    entry.id = entries_.size() + 1;
    entry.label = "pt" + QString::number(entry.id);
    entry.category = category;
    entry.axis = "XY";
    entry.x = xVal;
    entry.y = yVal;
    entry.density = 1;
    // Mark as outlier if far from center range
    entry.outlier = (xVal > 90 || xVal < 10 || yVal > 90 || yVal < 10);
    entry.color = color;

    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
    emit hexClicked(entry.id, entry.x, entry.y);
    inputField_->clear();
}

void PaperHexbinChart::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperHexbinChart::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render hexbin chart");
        return;
    }
    infoLabel_->setText(QString("%1 points | avg density: %2 | %3 outliers")
        .arg(entries_.size())
        .arg(avgDensity(), 0, 'f', 1)
        .arg(outlierCount()));
}

void PaperHexbinChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HexbinEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.axis     = settings_.value("axis").toString();
        e.x        = settings_.value("x").toDouble();
        e.y        = settings_.value("y").toDouble();
        e.density  = settings_.value("density").toInt();
        e.outlier  = settings_.value("outlier").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHexbinChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("axis",     entries_[i].axis);
        settings_.setValue("x",        entries_[i].x);
        settings_.setValue("y",        entries_[i].y);
        settings_.setValue("density",  entries_[i].density);
        settings_.setValue("outlier",  entries_[i].outlier);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}

QList<HexbinEntry> PaperHexbinChart::entries() const {
    return entries_;
}

int PaperHexbinChart::outlierCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.outlier) c++;
    return c;
}

qreal PaperHexbinChart::avgDensity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_)
        total += e.density;
    return total / entries_.size();
}

QMap<QString, int> PaperHexbinChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperHexbinChart::addEntry(const HexbinEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}
