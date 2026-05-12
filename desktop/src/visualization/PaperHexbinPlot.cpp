#include "visualization/PaperHexbinPlot.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <QtMath>

PaperHexbinPlot::PaperHexbinPlot(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperHexbinPlot")
{
    setupUI();
    loadSettings();
}

void PaperHexbinPlot::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Group A", "Group B", "Group C", "Group D", "Group E"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter label...");
    leftPanel->addWidget(inputField_);

    renderBtn_ = new QPushButton("Render", this);
    leftPanel->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel(this);
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();

    mainLayout->addLayout(leftPanel);
    mainLayout->addStretch(1);

    connect(renderBtn_, &QPushButton::clicked, this, &PaperHexbinPlot::onRender);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHexbinPlot::onClear);

    updateInfo();
}

void PaperHexbinPlot::loadSettings()
{
    settings_.beginGroup("HexbinPlot");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HexbinEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.label = settings_.value("label").toString();
        entry.category = settings_.value("category").toString();
        entry.group = settings_.value("group").toString();
        entry.x = settings_.value("x").toReal();
        entry.y = settings_.value("y").toReal();
        entry.count = settings_.value("count").toInt();
        entry.dense = settings_.value("dense").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperHexbinPlot::saveSettings()
{
    settings_.beginGroup("HexbinPlot");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_[i];
        settings_.setValue("id", entry.id);
        settings_.setValue("label", entry.label);
        settings_.setValue("category", entry.category);
        settings_.setValue("group", entry.group);
        settings_.setValue("x", entry.x);
        settings_.setValue("y", entry.y);
        settings_.setValue("count", entry.count);
        settings_.setValue("dense", entry.dense);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperHexbinPlot::addEntry(const HexbinEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<HexbinEntry> PaperHexbinPlot::entries() const
{
    return entries_;
}

int PaperHexbinPlot::denseCount() const
{
    int count = 0;
    for (const auto& entry : entries_) {
        if (entry.dense)
            ++count;
    }
    return count;
}

int PaperHexbinPlot::maxCount() const
{
    int maxVal = 0;
    for (const auto& entry : entries_) {
        if (entry.count > maxVal)
            maxVal = entry.count;
    }
    return maxVal;
}

QMap<QString, int> PaperHexbinPlot::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& entry : entries_) {
        counts[entry.category]++;
    }
    return counts;
}

void PaperHexbinPlot::onRender()
{
    QString label = inputField_->text().trimmed();
    if (label.isEmpty())
        label = QString("Hex_%1").arg(entries_.size() + 1);

    static const QStringList categories = {"Group A", "Group B", "Group C", "Group D", "Group E"};
    static const QStringList colorNames = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};

    QString selectedCategory = categoryCombo_->currentText();
    QString category;
    if (selectedCategory == "All") {
        int idx = QRandomGenerator::global()->bounded(categories.size());
        category = categories[idx];
    } else {
        category = selectedCategory;
    }

    int colorIndex = categories.indexOf(category);
    if (colorIndex < 0) colorIndex = 0;

    qreal x = QRandomGenerator::global()->generateDouble();
    qreal y = QRandomGenerator::global()->generateDouble();
    int count = QRandomGenerator::global()->bounded(1, 101);
    bool dense = count > 50;

    HexbinEntry entry;
    entry.id = entries_.size() + 1;
    entry.label = label;
    entry.category = category;
    entry.group = category;
    entry.x = x;
    entry.y = y;
    entry.count = count;
    entry.dense = dense;
    entry.color = QColor(colorNames[colorIndex]);

    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();

    emit hexSelected(entry.id, entry.count);
}

void PaperHexbinPlot::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperHexbinPlot::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // 3-column layout: hexbin view (60%), legend (20%), stats (20%)
    int col1Width = w * 6 / 10;
    int col2Width = w * 2 / 10;
    int col3Width = w - col1Width - col2Width;

    QRect hexRect(0, 0, col1Width, h);
    QRect legendRect(col1Width, 0, col2Width, h);
    QRect statsRect(col1Width + col2Width, 0, col3Width, h);

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    drawHexbinView(p, hexRect);
    drawCategoryLegend(p, legendRect);
    drawStats(p, statsRect);
}

void PaperHexbinPlot::drawHexbinView(QPainter& p, const QRect& rect)
{
    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Hexbin Plot");

    // Drawing area below the title
    int titleHeight = 35;
    QRect plotArea = rect.adjusted(10, titleHeight, -10, -10);

    // Border
    p.setPen(QPen(QColor("#cbd5e1"), 1));
    p.drawRect(plotArea);

    if (entries_.isEmpty())
        return;

    // Hexagonal grid parameters
    int hexSize = 20;
    int hexW = hexSize * 2;
    int hexH = static_cast<int>(hexSize * qSqrt(3));
    int cols = qMax(1, plotArea.width() / (hexW * 3 / 2));
    int rows = qMax(1, plotArea.height() / hexH);

    // Accumulate counts into grid cells
    QMap<QPair<int,int>, int> gridCounts;
    QMap<QPair<int,int>, QColor> gridColors;
    int maxVal = maxCount();

    for (const auto& entry : entries_) {
        int col = qBound(0, static_cast<int>(entry.x * cols), cols - 1);
        int row = qBound(0, static_cast<int>(entry.y * rows), rows - 1);
        auto key = qMakePair(col, row);
        gridCounts[key] += entry.count;
        if (!gridColors.contains(key)) {
            gridColors[key] = entry.color;
        }
    }

    // Draw hexagons
    QFont smallFont;
    smallFont.setPointSize(7);
    p.setFont(smallFont);

    for (auto it = gridCounts.constBegin(); it != gridCounts.constEnd(); ++it) {
        int col = it.key().first;
        int row = it.key().second;
        int count = it.value();
        QColor baseColor = gridColors.value(it.key(), QColor("#3b82f6"));

        // Compute alpha proportional to count
        int alpha = maxVal > 0 ? static_cast<int>(50 + 205.0 * count / maxVal) : 50;
        alpha = qBound(50, alpha, 255);

        // Hex center position
        qreal cx = plotArea.left() + hexSize + col * hexW * 1.5;
        qreal cy = plotArea.top() + hexH / 2.0 + row * hexH;
        if (col % 2 == 1)
            cy += hexH / 2.0;

        // Skip if outside plot area
        if (cx + hexSize > plotArea.right() || cy + hexH / 2.0 > plotArea.bottom())
            continue;

        QColor fillColor = baseColor;
        fillColor.setAlpha(alpha);

        // Draw hexagon path (flat-top)
        QPainterPath hexPath;
        for (int i = 0; i < 6; ++i) {
            qreal angle = M_PI / 3.0 * i;
            qreal px = cx + hexSize * qCos(angle);
            qreal py = cy + hexSize * qSin(angle);
            if (i == 0)
                hexPath.moveTo(px, py);
            else
                hexPath.lineTo(px, py);
        }
        hexPath.closeSubpath();

        p.setPen(QPen(QColor("#ffffff"), 1));
        p.setBrush(fillColor);
        p.drawPath(hexPath);

        // Count label inside hex
        p.setPen(QColor("#ffffff"));
        p.drawText(QRectF(cx - hexSize / 2, cy - hexSize / 4, hexSize, hexSize / 2),
                   Qt::AlignCenter, QString::number(count));
    }
}

void PaperHexbinPlot::drawCategoryLegend(QPainter& p, const QRect& rect)
{
    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 10, -5, 0), Qt::AlignLeft | Qt::AlignTop, "Legend");

    static const QStringList categories = {"Group A", "Group B", "Group C", "Group D", "Group E"};
    static const QStringList colorNames = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};

    QFont labelFont;
    labelFont.setPointSize(9);
    p.setFont(labelFont);

    int y = 40;
    int squareSize = 14;
    int margin = 10;

    QMap<QString, int> counts = categoryCounts();

    for (int i = 0; i < categories.size(); ++i) {
        int count = counts.value(categories[i], 0);

        // Colored square
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(colorNames[i]));
        p.drawRect(rect.left() + margin, y, squareSize, squareSize);

        // Label with count
        p.setPen(QColor("#334155"));
        p.drawText(QRect(rect.left() + margin + squareSize + 6, y - 2,
                         rect.width() - squareSize - margin - 12, squareSize + 4),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1 (%2)").arg(categories[i]).arg(count));

        y += squareSize + 10;
    }
}

void PaperHexbinPlot::drawStats(QPainter& p, const QRect& rect)
{
    // Title
    p.setPen(QColor("#1e293b"));
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 10, -5, 0), Qt::AlignLeft | Qt::AlignTop, "Stats");

    QFont statsFont;
    statsFont.setPointSize(9);
    p.setFont(statsFont);

    int y = 40;
    int lineH = 22;

    p.setPen(QColor("#334155"));
    p.drawText(rect.adjusted(10, y, -5, 0), Qt::AlignLeft | Qt::AlignTop,
               QString("Total Hexes: %1").arg(entries_.size()));
    y += lineH;

    p.drawText(rect.adjusted(10, y, -5, 0), Qt::AlignLeft | Qt::AlignTop,
               QString("Dense Count: %1").arg(denseCount()));
    y += lineH;

    p.drawText(rect.adjusted(10, y, -5, 0), Qt::AlignLeft | Qt::AlignTop,
               QString("Max Count: %1").arg(maxCount()));
}

void PaperHexbinPlot::updateInfo()
{
    infoLabel_->setText(QString("Hexes: %1 | Dense: %2 | Max: %3")
                            .arg(entries_.size())
                            .arg(denseCount())
                            .arg(maxCount()));
}
