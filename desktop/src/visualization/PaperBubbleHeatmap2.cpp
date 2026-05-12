#include "visualization/PaperBubbleHeatmap2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <cmath>

namespace {
static const QStringList kCells = {"A1", "B2", "C3", "D4"};
static const QStringList kAxes = {"X-Axis", "Y-Axis", "Z-Axis", "Time", "Frequency"};
static const QStringList kCategories = {"Spatial", "Temporal", "Spectral", "Behavioral", "Environmental"};
static const QColor kPalette[] = {
    QColor(59, 130, 246),   // #3b82f6
    QColor(22, 163, 74),    // #16a34a
    QColor(217, 119, 6),    // #d97706
    QColor(220, 38, 38),    // #dc2626
    QColor(124, 58, 237)    // #7c3aed
};
}

PaperBubbleHeatmap2::PaperBubbleHeatmap2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BubbleHeatmap2")
{
    setupUI();
    loadSettings();
}

void PaperBubbleHeatmap2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems(QStringList{"All"} + kCategories);
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search cells...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperBubbleHeatmap2::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBubbleHeatmap2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    auto* infoRow = new QHBoxLayout();
    infoRow->addStretch();
    infoLabel_ = new QLabel("Bubble Heatmap 2");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoRow->addWidget(infoLabel_);
    mainLayout->addLayout(infoRow);

    setMinimumSize(700, 550);
}

void PaperBubbleHeatmap2::addEntry(const BubbleHeatmap2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit cellSelected(entry.id, entry.intensity);
    update();
}

QList<BubbleHeatmap2Entry> PaperBubbleHeatmap2::entries() const {
    return entries_;
}

int PaperBubbleHeatmap2::hotspotCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.hotspot) ++c;
    return c;
}

qreal PaperBubbleHeatmap2::avgIntensity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.intensity;
    return sum / entries_.size();
}

QMap<QString, int> PaperBubbleHeatmap2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBubbleHeatmap2::onRender() {
    int id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    BubbleHeatmap2Entry e;
    e.id = id;
    e.cell = kCells[QRandomGenerator::global()->bounded(kCells.size())];
    e.category = kCategories[QRandomGenerator::global()->bounded(kCategories.size())];
    e.axis = kAxes[QRandomGenerator::global()->bounded(kAxes.size())];
    e.intensity = QRandomGenerator::global()->bounded(100) / 100.0;
    e.density = 5 + QRandomGenerator::global()->bounded(96);
    e.hotspot = e.intensity >= 0.75;
    int cIdx = kCategories.indexOf(e.category);
    if (cIdx < 0) cIdx = 0;
    e.color = kPalette[cIdx % 5];
    addEntry(e);
    inputField_->clear();
}

void PaperBubbleHeatmap2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Bubble Heatmap 2");
    update();
}

void PaperBubbleHeatmap2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render bubble heatmap 2");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 28, "Bubble Heatmap 2");

    int w = width(), h = height();
    int topY = 42;

    // Heatmap: left 60%
    int heatmapW = static_cast<int>(w * 0.60);
    int heatmapH = static_cast<int>(h * 0.75) - topY;
    drawHeatmap(p, QRect(10, topY, heatmapW - 10, heatmapH));

    // Category legend: right 40%
    int legendX = heatmapW;
    int legendW = w - heatmapW - 10;
    drawCategoryLegend(p, QRect(legendX, topY, legendW, heatmapH));

    // Stats: bottom 25%
    int statsY = topY + heatmapH + 8;
    int statsH = h - statsY - 8;
    drawStats(p, QRect(10, statsY, w - 20, statsH));
}

void PaperBubbleHeatmap2::drawHeatmap(QPainter& p, const QRect& r) {
    // Grid background
    p.setPen(QPen(QColor(226, 232, 240), 1));
    int gridCols = 6, gridRows = 6;
    for (int gx = 0; gx <= gridCols; ++gx) {
        int x = r.x() + gx * r.width() / gridCols;
        p.drawLine(x, r.y(), x, r.y() + r.height());
    }
    for (int gy = 0; gy <= gridRows; ++gy) {
        int y = r.y() + gy * r.height() / gridRows;
        p.drawLine(r.x(), y, r.x() + r.width(), y);
    }

    // Determine max density for scaling
    int maxDensity = 1;
    for (const auto& e : entries_)
        maxDensity = qMax(maxDensity, e.density);

    // Place each entry on grid based on cell and axis
    for (const auto& e : entries_) {
        int colIdx = kCells.indexOf(e.cell);
        int rowIdx = kAxes.indexOf(e.axis);
        if (colIdx < 0) colIdx = 0;
        if (rowIdx < 0) rowIdx = 0;

        qreal cellW = static_cast<qreal>(r.width()) / kCells.size();
        qreal cellH = static_cast<qreal>(r.height()) / kAxes.size();
        int cx = r.x() + static_cast<int>((colIdx + 0.5) * cellW);
        int cy = r.y() + static_cast<int>((rowIdx + 0.5) * cellH);

        int baseRadius = qMax(8, static_cast<int>((static_cast<qreal>(e.density) / maxDensity) * 28));
        int radius = baseRadius;

        // Hotspot pulsation: larger radius + glow
        if (e.hotspot) {
            radius = static_cast<int>(baseRadius * 1.35);
            QRadialGradient glow(cx, cy, radius * 2.0);
            QColor glowColor = e.color;
            glowColor.setAlpha(60);
            glow.setColorAt(0.0, glowColor);
            glowColor.setAlpha(0);
            glow.setColorAt(1.0, glowColor);
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawEllipse(cx - radius * 2, cy - radius * 2, radius * 4, radius * 4);
        }

        // Color by intensity: cool blue (0.0) to hot red (1.0)
        int blue = static_cast<int>(255 * (1.0 - e.intensity));
        int red = static_cast<int>(255 * e.intensity);
        QColor intensityColor(red, 30, blue);

        int alpha = e.hotspot ? 210 : 140;
        QColor fillColor = QColor(intensityColor.red(), intensityColor.green(),
                                  intensityColor.blue(), alpha);
        p.setPen(e.hotspot ? QPen(e.color, 2) : QPen(intensityColor.lighter(140), 1));
        p.setBrush(fillColor);
        p.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

        // Cell label inside bubble
        if (radius > 10) {
            p.setPen(alpha > 160 ? Qt::white : QColor(15, 23, 42));
            p.setFont(QFont("Arial", qMin(8, radius / 3), QFont::Bold));
            p.drawText(cx - radius, cy - 4, radius * 2, 10, Qt::AlignCenter, e.cell);
        }
    }

    // Axis labels on left edge
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    qreal cellH = static_cast<qreal>(r.height()) / kAxes.size();
    for (int i = 0; i < kAxes.size(); ++i) {
        int ly = r.y() + static_cast<int>((i + 0.5) * cellH);
        p.drawText(r.x() - 2, ly - 6, 2, 12, Qt::AlignRight | Qt::AlignVCenter, kAxes[i]);
    }

    // Cell labels on bottom
    qreal cellW = static_cast<qreal>(r.width()) / kCells.size();
    for (int i = 0; i < kCells.size(); ++i) {
        int lx = r.x() + static_cast<int>((i + 0.5) * cellW);
        p.drawText(lx - 20, r.y() + r.height() + 2, 40, 14,
                   Qt::AlignCenter, kCells[i]);
    }
}

void PaperBubbleHeatmap2::drawCategoryLegend(QPainter& p, const QRect& r) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(r.topLeft(), "Categories");

    auto counts = categoryCounts();
    int itemH = qMin(30, (r.height() - 30) / kCategories.size());

    for (int i = 0; i < kCategories.size(); ++i) {
        int y = r.y() + 22 + i * (itemH + 6);
        int count = counts.contains(kCategories[i]) ? counts[kCategories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i]);
        p.drawEllipse(r.x() + 5, y + 4, 14, 14);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(r.x() + 24, y + 2, r.width() / 2 - 24, 18,
                   Qt::AlignVCenter, kCategories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(r.x() + r.width() / 2, y + 2, r.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");
    }
}

void PaperBubbleHeatmap2::drawStats(QPainter& p, const QRect& r) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Cells",    QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Hotspot Count",  QString::number(hotspotCount()), QColor(220, 38, 38)},
        {"Avg Intensity",  QString::number(avgIntensity(), 'f', 2), QColor(22, 163, 74)},
        {"Total Density",  QString::number(
            [](const QList<BubbleHeatmap2Entry>& es) {
                int t = 0; for (const auto& e : es) t += e.density; return t;
            }(entries_)), QColor(124, 58, 237)}
    };

    int n = stats.size();
    int gap = 10;
    int boxW = (r.width() - (n - 1) * gap) / n;
    int boxH = qMin(48, r.height() - 4);

    for (int i = 0; i < n; ++i) {
        int x = r.x() + i * (boxW + gap);
        int y = r.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, y + 4, boxW - 20, 24, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + 28, boxW - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperBubbleHeatmap2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Bubble Heatmap 2");
        return;
    }
    infoLabel_->setText(QString("%1 cells | %2 hotspots | avg %3")
        .arg(entries_.size())
        .arg(hotspotCount())
        .arg(avgIntensity(), 0, 'f', 2));
}

void PaperBubbleHeatmap2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BubbleHeatmap2Entry e;
        e.id        = settings_.value("id").toInt();
        e.cell      = settings_.value("cell").toString();
        e.category  = settings_.value("category").toString();
        e.axis      = settings_.value("axis").toString();
        e.intensity = settings_.value("intensity").toDouble();
        e.density   = settings_.value("density").toInt();
        e.hotspot   = settings_.value("hotspot").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    // Seed 8 demo entries if empty
    if (entries_.isEmpty()) {
        for (int i = 0; i < 8; ++i) {
            BubbleHeatmap2Entry e;
            e.id = i + 1;
            e.cell = kCells[i % kCells.size()];
            e.category = kCategories[i % kCategories.size()];
            e.axis = kAxes[i % kAxes.size()];
            e.intensity = (30 + (i * 11) % 70) / 100.0;
            e.density = 10 + (i * 13) % 90;
            e.hotspot = e.intensity >= 0.75;
            int cIdx = kCategories.indexOf(e.category);
            if (cIdx < 0) cIdx = 0;
            e.color = kPalette[cIdx % 5];
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperBubbleHeatmap2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",        entries_[i].id);
        settings_.setValue("cell",      entries_[i].cell);
        settings_.setValue("category",  entries_[i].category);
        settings_.setValue("axis",      entries_[i].axis);
        settings_.setValue("intensity", entries_[i].intensity);
        settings_.setValue("density",   entries_[i].density);
        settings_.setValue("hotspot",   entries_[i].hotspot);
        settings_.setValue("color",     entries_[i].color.name());
    }
    settings_.endArray();
}
