#include "visualization/PaperPolarChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QtMath>

namespace {
static const QColor kPalette[] = {
    QColor(0x3b, 0x82, 0xf6),  // #3b82f6
    QColor(0x16, 0xa3, 0x4a),  // #16a34a
    QColor(0xd9, 0x77, 0x06),  // #d97706
    QColor(0xdc, 0x26, 0x26),  // #dc2626
    QColor(0x7c, 0x3a, 0xed),  // #7c3aed
};
static constexpr int kPaletteSize = 5;
} // anonymous namespace

PaperPolarChart::PaperPolarChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PolarChart")
{
    setupUI();
    loadSettings();
}

// -- public API --

void PaperPolarChart::addEntry(const PolarEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit polarRendered(entry.id, entry.value);
    update();
}

QList<PolarEntry> PaperPolarChart::entries() const { return entries_; }

int PaperPolarChart::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dominant) c++;
    return c;
}

qreal PaperPolarChart::maxValue() const {
    if (entries_.isEmpty()) return 0;
    qreal mx = entries_[0].value;
    for (const auto& e : entries_) mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperPolarChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// -- slots --

void PaperPolarChart::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    entries_.clear();

    // Parse comma-separated "label:value" pairs
    QStringList tokens = text.split(',', Qt::SkipEmptyParts);
    if (tokens.isEmpty()) return;

    int n = tokens.size();
    qreal angleStep = 360.0 / n;
    qreal nextAngle = 0.0;

    // First pass: collect values to determine dominant threshold (top 20%)
    QVector<qreal> rawValues;
    rawValues.reserve(n);
    for (const auto& tok : tokens) {
        QStringList parts = tok.split(':');
        qreal v = (parts.size() >= 2) ? parts[1].trimmed().toDouble() : 0.0;
        if (v <= 0) v = 1.0;
        rawValues << v;
    }

    // Sort descending to find top-20% threshold
    QVector<qreal> sorted = rawValues;
    std::sort(sorted.begin(), sorted.end(), std::greater<qreal>());
    int topN = qMax(1, static_cast<int>(std::ceil(n * 0.2)));
    qreal threshold = sorted[qMin(topN - 1, sorted.size() - 1)];

    // Second pass: build entries
    QString selectedCategory = categoryCombo_->currentText().toLower();
    if (selectedCategory == "all") selectedCategory.clear();

    for (int i = 0; i < n; ++i) {
        QStringList parts = tokens[i].split(':');
        PolarEntry e;
        e.id = i + 1;
        e.label = parts[0].trimmed();
        e.value = rawValues[i];
        e.category = selectedCategory.isEmpty()
                         ? (QStringList{"alpha", "beta", "gamma"}[i % 3])
                         : selectedCategory;
        e.group = e.category;
        e.angle = nextAngle;
        e.radius = e.value;
        e.dominant = (e.value >= threshold);
        e.color = kPalette[i % kPaletteSize];
        entries_.append(e);

        nextAngle += angleStep;
    }

    saveSettings();
    updateInfo();

    // Emit for each entry
    for (const auto& e : entries_) {
        emit polarRendered(e.id, e.value);
    }

    inputField_->clear();
    update();
}

void PaperPolarChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Enter label:value pairs to render polar chart");
    update();
}

// -- painting --

void PaperPolarChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Enter label:value pairs to render polar chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Polar Chart");

    int w = width(), h = height();
    drawPolarView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperPolarChart::drawPolarView(QPainter& p, const QRect& chartRect) {
    int n = entries_.size();
    if (n == 0) return;

    qreal maxVal = maxValue();
    if (maxVal <= 0) maxVal = 100.0;

    int side = qMin(chartRect.width(), chartRect.height()) - 40;
    if (side <= 0) return;
    qreal cx = chartRect.x() + chartRect.width() / 2.0;
    qreal cy = chartRect.y() + chartRect.height() / 2.0;
    qreal maxR = side / 2.0;

    // Draw concentric guide circles
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DotLine));
    p.setBrush(Qt::NoBrush);
    for (int ring = 1; ring <= 4; ++ring) {
        qreal r = maxR * ring / 4.0;
        p.drawEllipse(QPointF(cx, cy), r, r);
    }

    // Draw cross-hairs
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    p.drawLine(QPointF(cx - maxR, cy), QPointF(cx + maxR, cy));
    p.drawLine(QPointF(cx, cy - maxR), QPointF(cx, cy + maxR));

    // Draw radial bars
    qreal angleStep = 360.0 / n;

    for (int i = 0; i < n; ++i) {
        const PolarEntry& e = entries_[i];
        qreal barRadius = (e.value / maxVal) * maxR;
        qreal startDeg = e.angle; // degrees, clockwise from 12 o'clock
        // QPainter uses 1/16th degree, 0 at 3 o'clock
        qreal qtStart = (90.0 - startDeg - angleStep) * 16.0;
        qreal qtSpan = angleStep * 16.0;

        // Filled wedge
        QPainterPath wedge;
        wedge.moveTo(cx, cy);
        wedge.arcTo(QRectF(cx - barRadius, cy - barRadius,
                           barRadius * 2, barRadius * 2),
                    qtStart / 16.0, -angleStep);
        wedge.closeSubpath();

        QColor fill = e.color;
        fill.setAlpha(e.dominant ? 160 : 90);
        p.setPen(QPen(e.color, e.dominant ? 2 : 1));
        p.setBrush(fill);
        p.drawPath(wedge);

        // Outer arc stroke for dominant entries
        if (e.dominant) {
            p.setPen(QPen(e.color.darker(120), 3));
            p.setBrush(Qt::NoBrush);
            p.drawArc(QRectF(cx - barRadius, cy - barRadius,
                             barRadius * 2, barRadius * 2),
                      static_cast<int>(qtStart), static_cast<int>(-qtSpan));
        }

        // Label at outer edge
        qreal labelAngle = qDegreesToRadians(startDeg + angleStep / 2.0);
        qreal labelR = maxR + 14;
        qreal lx = cx + labelR * qCos(labelAngle);
        qreal ly = cy - labelR * qSin(labelAngle);

        p.setPen(e.dominant ? QColor(15, 23, 42) : QColor(100, 116, 139));
        p.setFont(e.dominant ? QFont("Arial", 8, QFont::Bold) : QFont("Arial", 7));
        p.drawText(QRectF(lx - 30, ly - 8, 60, 16), Qt::AlignCenter, e.label);

        // Value text near bar tip
        qreal valR = barRadius * 0.55;
        qreal vx = cx + valR * qCos(labelAngle);
        qreal vy = cy - valR * qSin(labelAngle);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRectF(vx - 18, vy - 7, 36, 14), Qt::AlignCenter,
                   QString::number(e.value, 'f', 0));
    }

    // Center dot
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(15, 23, 42));
    p.drawEllipse(QPointF(cx, cy), 4, 4);
}

void PaperPolarChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = counts.keys();

    if (cats.isEmpty()) {
        cats = QStringList{"alpha", "beta", "gamma"};
    }

    QString displayLabels[] = {"Alpha", "Beta", "Gamma", "Delta", "Epsilon"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int catCount = qMin(cats.size(), 5);
    int barH = qMin(22, (rect.height() - 30) / qMax(catCount, 1));

    for (int i = 0; i < catCount; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        QString label = (i < 5) ? displayLabels[i] : cats[i];
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, label);

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i % 5]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperPolarChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",   QString::number(entries_.size()),    QColor(0x3b, 0x82, 0xf6)},
        {"Dominant",  QString::number(dominantCount()),    QColor(0x16, 0xa3, 0x4a)},
        {"Max Value", QString::number(maxValue(), 'f', 1), QColor(0xd9, 0x77, 0x06)},
        {"Avg Value", QString::number(
             entries_.isEmpty() ? 0.0
                 : std::accumulate(entries_.begin(), entries_.end(), 0.0,
                       [](double s, const PolarEntry& e) { return s + e.value; })
                 / entries_.size(), 'f', 1),
         QColor(0x7c, 0x3a, 0xed)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0xdc, 0x26, 0x26)}
    };

    int boxH = qMin(36, (rect.height() - 10) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 4);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 13, QFont::Bold));
        p.drawText(rect.x() + 10, y + 3, rect.width() - 20, 20,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 10, y + 20, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

// -- helpers --

void PaperPolarChart::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Enter label:value pairs to render polar chart");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 dominant | max %3")
        .arg(entries_.size())
        .arg(dominantCount())
        .arg(maxValue(), 0, 'f', 1));
}

// -- settings --

void PaperPolarChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PolarEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group    = settings_.value("group").toString();
        e.angle    = settings_.value("angle").toDouble();
        e.radius   = settings_.value("radius").toDouble();
        e.value    = settings_.value("value").toDouble();
        e.dominant = settings_.value("dominant").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperPolarChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group",    entries_[i].group);
        settings_.setValue("angle",    entries_[i].angle);
        settings_.setValue("radius",   entries_[i].radius);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("dominant", entries_[i].dominant);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}

// -- UI setup --

void PaperPolarChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperPolarChart::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Alpha", "Beta", "Gamma"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPolarChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label:value pairs, comma-separated (e.g. A:10, B:30, C:20)");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Enter label:value pairs to render polar chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}
