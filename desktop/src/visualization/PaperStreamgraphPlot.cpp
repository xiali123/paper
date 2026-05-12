#include "visualization/PaperStreamgraphPlot.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

namespace {
static const QStringList kColors = {
    "#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"
};
static const QStringList kCategories = {
    "Category-A", "Category-B", "Category-C", "Category-D", "Category-E"
};
}

PaperStreamgraphPlot::PaperStreamgraphPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "StreamgraphPlot")
{
    setupUI();
    loadSettings();
}

void PaperStreamgraphPlot::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems(kCategories);
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperStreamgraphPlot::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStreamgraphPlot::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render a streamgraph");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(700, 500);
}

void PaperStreamgraphPlot::addEntry(const StreamgraphEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<StreamgraphEntry> PaperStreamgraphPlot::entries() const {
    return entries_;
}

int PaperStreamgraphPlot::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.dominant) c++;
    return c;
}

qreal PaperStreamgraphPlot::maxValue() const {
    qreal mx = 0.0;
    for (const auto& e : entries_)
        if (e.value > mx) mx = e.value;
    return mx;
}

QMap<QString, int> PaperStreamgraphPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperStreamgraphPlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    StreamgraphEntry e;
    e.id = entries_.size() + 1;
    e.label = text;
    e.category = categoryCombo_->currentText();
    e.group = "group-" + QString::number(QRandomGenerator::global()->bounded(1, 4));
    e.value = QRandomGenerator::global()->bounded(10000) / 100.0;
    e.baseline = QRandomGenerator::global()->bounded(5000) / 100.0;
    e.rank = entries_.size();
    e.dominant = e.value > 70.0;

    int cIdx = categoryCombo_->currentIndex();
    e.color = QColor(kColors[cIdx % kColors.size()]);

    entries_.append(e);
    saveSettings();
    updateInfo();
    update();
    emit streamSelected(e.id, e.value);
    inputField_->clear();
}

void PaperStreamgraphPlot::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperStreamgraphPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render a streamgraph");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Streamgraph");

    int w = width(), h = height();
    int colW = w / 3;

    drawStreamgraph(p, QRect(20, 50, colW - 10, h - 80));
    drawCategoryLegend(p, QRect(colW + 10, 50, colW - 20, h / 2 - 30));
    drawStats(p, QRect(2 * colW + 10, 50, colW - 30, h - 80));
}

void PaperStreamgraphPlot::drawStreamgraph(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;
    int baseX = rect.x() + margin;
    int baseY = rect.y() + margin;

    // Draw axes
    p.setPen(QColor(203, 213, 225));
    p.drawLine(baseX, baseY, baseX, baseY + plotH);
    p.drawLine(baseX, baseY + plotH, baseX + plotW, baseY + plotH);

    // Compute stacked positions
    int n = entries_.size();
    if (n == 0) return;

    // Build cumulative upper bounds for each entry across evenly spaced x points
    int numPoints = qMax(n * 4, 20);
    QVector<qreal> cumLower(numPoints, 0.0);
    QVector<qreal> cumUpper(numPoints, 0.0);

    // Collect per-stream data: each stream contributes a symmetric band
    struct StreamBand {
        QVector<qreal> lower;
        QVector<qreal> upper;
        QColor color;
    };
    QList<StreamBand> bands;

    qreal maxCum = 0.0;
    QVector<qreal> runningUpper(numPoints, 0.0);

    for (int s = 0; s < n; ++s) {
        const auto& entry = entries_[s];
        StreamBand band;
        band.color = entry.color;
        band.lower.resize(numPoints);
        band.upper.resize(numPoints);

        qreal halfH = (entry.value / 100.0) * (plotH / 2.0);
        qreal baseOff = (entry.baseline / 100.0) * (plotH / 4.0);

        for (int i = 0; i < numPoints; ++i) {
            qreal t = static_cast<qreal>(i) / (numPoints - 1);
            // Smooth bell-like shape using cosine curve centered at stream's rank position
            qreal center = static_cast<qreal>(s) / qMax(n - 1, 1);
            qreal dist = qAbs(t - center);
            qreal shape = qMax(0.0, qCos(dist * M_PI * 1.2));
            qreal localH = halfH * shape;

            band.lower[i] = runningUpper[i] + baseOff * (1.0 - shape);
            band.upper[i] = band.lower[i] + localH;
            runningUpper[i] = band.upper[i];

            if (runningUpper[i] > maxCum) maxCum = runningUpper[i];
        }
        bands.append(band);
    }

    // Normalize to fit plot area
    qreal scale = maxCum > 0 ? (plotH / 2.0) / maxCum : 1.0;

    // Draw streams from top to bottom for layering
    for (int s = 0; s < bands.size(); ++s) {
        const StreamBand& band = bands[s];
        QColor fill = band.color;
        fill.setAlphaF(0.70);

        QPainterPath path;
        // Top edge (left to right)
        path.moveTo(baseX,
                     baseY + plotH / 2.0 - band.upper[0] * scale);
        for (int i = 1; i < numPoints; ++i) {
            qreal x = baseX + (static_cast<qreal>(i) / (numPoints - 1)) * plotW;
            qreal y = baseY + plotH / 2.0 - band.upper[i] * scale;
            qreal prevX = baseX + (static_cast<qreal>(i - 1) / (numPoints - 1)) * plotW;
            qreal prevY = baseY + plotH / 2.0 - band.upper[i - 1] * scale;
            qreal cpx = (prevX + x) / 2.0;
            path.cubicTo(cpx, prevY, cpx, y, x, y);
        }
        // Bottom edge (right to left)
        for (int i = numPoints - 1; i >= 0; --i) {
            qreal x = baseX + (static_cast<qreal>(i) / (numPoints - 1)) * plotW;
            qreal y = baseY + plotH / 2.0 - band.lower[i] * scale;
            if (i == numPoints - 1) {
                path.lineTo(x, y);
            } else {
                qreal nextX = baseX + (static_cast<qreal>(i + 1) / (numPoints - 1)) * plotW;
                qreal nextY = baseY + plotH / 2.0 - band.lower[i + 1] * scale;
                qreal cpx = (nextX + x) / 2.0;
                path.cubicTo(cpx, nextY, cpx, y, x, y);
            }
        }
        path.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(path);

        // Thin outline
        p.setPen(QPen(band.color.darker(120), 0.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }

    // Center baseline
    p.setPen(QPen(QColor(100, 116, 139), 0.5, Qt::DashLine));
    p.drawLine(baseX, baseY + plotH / 2, baseX + plotW, baseY + plotH / 2);

    // Title
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x() + 5, rect.y() + 14, "Streamgraph");
}

void PaperStreamgraphPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Legend");

    auto counts = categoryCounts();
    int itemH = qMin(28, (rect.height() - 50) / qMax(kCategories.size(), 1));

    for (int i = 0; i < kCategories.size(); ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(kCategories[i]) ? counts[kCategories[i]] : 0;

        // Colored square
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(kColors[i]));
        p.drawRect(rect.x() + 5, y + 4, 14, 14);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, kCategories[i]);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " streams");
    }
}

void PaperStreamgraphPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };
    QList<Stat> stats = {
        {"Total Streams", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Dominant",      QString::number(dominantCount()), QColor(220, 38, 38)},
        {"Max Value",     QString::number(maxValue(), 'f', 1), QColor(124, 58, 237)}
    };

    int boxH = qMin(42, (rect.height() - 10) / qMax(stats.size(), 1));
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

void PaperStreamgraphPlot::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render a streamgraph");
        return;
    }
    infoLabel_->setText(QString("Streams: %1 | Dominant: %2 | Max: %3")
        .arg(entries_.size())
        .arg(dominantCount())
        .arg(maxValue(), 0, 'f', 1));
}

void PaperStreamgraphPlot::loadSettings() {
    settings_.beginGroup("StreamgraphPlot");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StreamgraphEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.value = settings_.value("value").toDouble();
        e.baseline = settings_.value("baseline").toDouble();
        e.rank = settings_.value("rank").toInt();
        e.dominant = settings_.value("dominant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperStreamgraphPlot::saveSettings() {
    settings_.beginGroup("StreamgraphPlot");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("baseline", entries_[i].baseline);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("dominant", entries_[i].dominant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
