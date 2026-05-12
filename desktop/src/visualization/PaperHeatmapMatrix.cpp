#include "visualization/PaperHeatmapMatrix.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperHeatmapMatrix::PaperHeatmapMatrix(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HeatmapMatrix")
{
    setupUI();
    loadSettings();
}

void PaperHeatmapMatrix::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperHeatmapMatrix::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Correlation", "Similarity", "Co-occurrence", "Frequency", "Distance"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHeatmapMatrix::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter matrix label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render heatmap matrix visualization");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperHeatmapMatrix::addEntry(const HeatEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit heatmapRendered(entry.id, entry.value);
    update();
}

QList<HeatEntry> PaperHeatmapMatrix::entries() const { return entries_; }

int PaperHeatmapMatrix::hotCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.value > 80) c++;
    return c;
}

qreal PaperHeatmapMatrix::maxValue() const {
    qreal mx = 0;
    for (const auto& e : entries_) mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperHeatmapMatrix::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperHeatmapMatrix::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Correlation", "Similarity", "Co-occurrence", "Frequency", "Distance"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();

    int gridSize = 6 + QRandomGenerator::global()->bounded(4); // 6-9 rows/cols
    QString cat = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};

    for (int r = 0; r < gridSize; ++r) {
        for (int c = 0; c < gridSize; ++c) {
            HeatEntry e;
            e.id = entries_.size() + 1;
            e.row = r;
            e.col = c;
            e.category = cat;
            e.value = QRandomGenerator::global()->bounded(1000) / 10.0;
            e.normalized = e.value / 100.0;
            e.rank = 0;
            e.hot = (e.value > 80);
            e.color = colors[r % colors.size()];
            entries_.append(e);
        }
    }

    // Compute ranks
    QList<qreal> values;
    for (const auto& e : entries_) values.append(e.value);
    std::sort(values.begin(), values.end(), std::greater<qreal>());
    for (auto& e : entries_) {
        for (int i = 0; i < values.size(); ++i) {
            if (qFuzzyCompare(values[i], e.value)) {
                e.rank = i + 1;
                break;
            }
        }
    }

    saveSettings();
    updateInfo();
    emit heatmapRendered(entries_.size(), entries_.last().value);
    update();
    inputField_->clear();
}

void PaperHeatmapMatrix::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render heatmap matrix visualization");
    update();
}

void PaperHeatmapMatrix::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render heatmap matrix visualization");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Heatmap Matrix");

    int w = width(), h = height();
    drawHeatmapView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperHeatmapMatrix::drawHeatmapView(QPainter& p, const QRect& rect) {
    if (entries_.isEmpty()) return;

    int margin = 10;
    // Determine grid dimensions
    int maxRow = 0, maxCol = 0;
    for (const auto& e : entries_) {
        maxRow = qMax(maxRow, e.row);
        maxCol = qMax(maxCol, e.col);
    }
    int rows = maxRow + 1;
    int cols = maxCol + 1;

    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;
    int cellW = plotW / cols;
    int cellH = plotH / rows;

    for (const auto& e : entries_) {
        int x = rect.x() + margin + e.col * cellW;
        int y = rect.y() + margin + e.row * cellH;

        // Interpolate color from blue (low) to red (high)
        qreal t = e.normalized; // 0.0 to 1.0
        int r = static_cast<int>(59 + t * (220 - 59));   // 0x3b -> 0xdc
        int g = static_cast<int>(130 - t * (130 - 38));  // 0x82 -> 0x26
        int b = static_cast<int>(246 - t * (246 - 38));  // 0xf6 -> 0x26
        QColor cellColor = QColor(
            qBound(0, r, 255),
            qBound(0, g, 255),
            qBound(0, b, 255)
        );

        p.setPen(QColor(255, 255, 255, 80));
        p.setBrush(cellColor);
        p.drawRect(x, y, cellW, cellH);

        // Draw value text in cell if large enough
        if (cellW > 30 && cellH > 20) {
            p.setPen(t > 0.5 ? Qt::white : QColor(15, 23, 42));
            p.setFont(QFont("Arial", 7));
            p.drawText(x, y, cellW, cellH, Qt::AlignCenter, QString::number(e.value, 'f', 0));
        }
    }
}

void PaperHeatmapMatrix::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Legend");

    // Color scale legend
    int y = rect.y() + 22;
    int scaleW = rect.width() - 20;
    int scaleH = 18;

    // Draw gradient bar
    for (int i = 0; i < scaleW; ++i) {
        qreal t = static_cast<qreal>(i) / scaleW;
        int r = static_cast<int>(59 + t * (220 - 59));
        int g = static_cast<int>(130 - t * (130 - 38));
        int b = static_cast<int>(246 - t * (246 - 38));
        p.setPen(QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255)));
        p.drawLine(rect.x() + 10 + i, y, rect.x() + 10 + i, y + scaleH);
    }

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + 10, y + scaleH + 12, "Low (0)");
    p.drawText(rect.x() + 10 + scaleW - 40, y + scaleH + 12, "High (100)");

    // Category counts
    auto counts = categoryCounts();
    y += scaleH + 30;
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), y, "Categories:");
    y += 18;

    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxCount = qMax(maxCount, it.value());
    if (maxCount == 0) maxCount = 1;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QColor c = colors[ci++ % colors.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * (rect.width() - 100));
        p.drawRoundedRect(rect.x() + 5, y, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 11, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
    }
}

void PaperHeatmapMatrix::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Cells", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Hot (>80)", QString::number(hotCount()), QColor(0xdc2626)},
        {"Max Value", QString::number(maxValue(), 'f', 1), QColor(0xd97706)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c3aed)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperHeatmapMatrix::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render heatmap matrix visualization");
        return;
    }
    infoLabel_->setText(QString("Cells: %1 | Hot: %2 | Max: %3")
        .arg(entries_.size())
        .arg(hotCount())
        .arg(QString::number(maxValue(), 'f', 1)));
}

void PaperHeatmapMatrix::loadSettings() {
    settings_.beginGroup("HeatmapMatrix");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        HeatEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.row = settings_.value(QString("row_%1").arg(i)).toInt();
        e.col = settings_.value(QString("col_%1").arg(i)).toInt();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.value = settings_.value(QString("value_%1").arg(i)).toDouble();
        e.normalized = settings_.value(QString("normalized_%1").arg(i)).toDouble();
        e.rank = settings_.value(QString("rank_%1").arg(i)).toInt();
        e.hot = settings_.value(QString("hot_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperHeatmapMatrix::saveSettings() {
    settings_.beginGroup("HeatmapMatrix");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("row_%1").arg(i), e.row);
        settings_.setValue(QString("col_%1").arg(i), e.col);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("value_%1").arg(i), e.value);
        settings_.setValue(QString("normalized_%1").arg(i), e.normalized);
        settings_.setValue(QString("rank_%1").arg(i), e.rank);
        settings_.setValue(QString("hot_%1").arg(i), e.hot);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
