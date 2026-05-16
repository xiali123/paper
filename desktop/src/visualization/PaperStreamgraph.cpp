#include "visualization/PaperStreamgraph.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

namespace {
static const QStringList kCategories = {
    "All", "Research", "Development", "Testing", "Deployment"
};

static const QMap<QString, QColor> kCategoryColors = {
    {"Research",    QColor("#3b82f6")},
    {"Development", QColor("#16a34a")},
    {"Testing",     QColor("#7c3aed")},
    {"Deployment",  QColor("#d97706")}
};

static QColor colorForCategory(const QString& cat) {
    auto it = kCategoryColors.find(cat);
    return it != kCategoryColors.end() ? it.value() : QColor("#64748b");
}
} // namespace

PaperStreamgraph::PaperStreamgraph(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "Streamgraph")
{
    setupUI();
    loadSettings();
}

void PaperStreamgraph::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems(kCategories);
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter stream...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperStreamgraph::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStreamgraph::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render a streamgraph");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(700, 500);
}

void PaperStreamgraph::addEntry(const StreamgraphEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<StreamgraphEntry> PaperStreamgraph::entries() const {
    return entries_;
}

int PaperStreamgraph::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.dominant) ++c;
    return c;
}

qreal PaperStreamgraph::avgValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.value;
    return sum / entries_.size();
}

QMap<QString, int> PaperStreamgraph::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperStreamgraph::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    StreamgraphEntry e;
    e.id = entries_.size() + 1;
    e.stream = text;
    e.category = categoryCombo_->currentText();
    if (e.category == "All") e.category = "Research";
    e.period = QString("P%1").arg(entries_.size() + 1);
    e.value = QRandomGenerator::global()->bounded(10000) / 100.0;
    e.peaks = QRandomGenerator::global()->bounded(1, 6);
    e.dominant = e.value > 70.0;
    e.color = colorForCategory(e.category);

    entries_.append(e);
    saveSettings();
    updateInfo();
    update();
    emit streamSelected(e.id, e.value);
    inputField_->clear();
}

void PaperStreamgraph::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperStreamgraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render a streamgraph");
        return;
    }

    int w = width();
    int h = height();

    // Top half: streamgraph chart
    drawStreamgraph(p, QRect(10, 10, w - 20, h / 2 - 20));
    // Bottom-left quarter: category legend
    drawCategoryLegend(p, QRect(10, h / 2 + 5, w / 2 - 20, h / 2 - 30));
    // Bottom-right quarter: statistics
    drawStats(p, QRect(w / 2 + 5, h / 2 + 5, w / 2 - 20, h / 2 - 30));
}

void PaperStreamgraph::drawStreamgraph(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;
    int baseX = rect.x() + margin;
    int baseY = rect.y() + margin;

    // Axes
    p.setPen(QColor(203, 213, 225));
    p.drawLine(baseX, baseY, baseX, baseY + plotH);
    p.drawLine(baseX, baseY + plotH, baseX + plotW, baseY + plotH);

    int n = entries_.size();
    if (n == 0) return;

    int numPoints = qMax(n * 4, 20);

    // Build per-stream bands stacked on a center baseline
    struct StreamBand {
        QVector<qreal> lower;
        QVector<qreal> upper;
        QColor color;
        bool dominant;
        qreal value;
    };
    QList<StreamBand> bands;

    QVector<qreal> runningUpper(numPoints, 0.0);
    QVector<qreal> runningLower(numPoints, 0.0);
    qreal maxCum = 0.0;

    for (int s = 0; s < n; ++s) {
        const auto& entry = entries_[s];
        StreamBand band;
        band.color = entry.color;
        band.dominant = entry.dominant;
        band.value = entry.value;
        band.lower.resize(numPoints);
        band.upper.resize(numPoints);

        qreal halfH = (entry.value / 100.0) * (plotH / 2.0);

        for (int i = 0; i < numPoints; ++i) {
            qreal t = static_cast<qreal>(i) / (numPoints - 1);
            qreal center = static_cast<qreal>(s) / qMax(n - 1, 1);
            qreal dist = qAbs(t - center);
            // Smooth bell-like shape centered at stream position
            qreal shape = qMax(0.0, qCos(dist * M_PI * 1.2));
            qreal localH = halfH * shape;

            band.lower[i] = runningLower[i];
            band.upper[i] = runningUpper[i] + localH;
            runningUpper[i] = band.upper[i];

            qreal peakAbs = qAbs(runningUpper[i] - runningLower[i]);
            if (peakAbs > maxCum) maxCum = peakAbs;
        }
        bands.append(band);
    }

    // Also track total stack height for normalization
    qreal maxStack = 0.0;
    for (int i = 0; i < numPoints; ++i) {
        qreal total = runningUpper[i];
        if (total > maxStack) maxStack = total;
    }

    qreal scale = maxStack > 0 ? (plotH / 2.0) / maxStack : 1.0;
    qreal midY = baseY + plotH / 2.0;

    // Draw each stream band
    for (int s = 0; s < bands.size(); ++s) {
        const StreamBand& band = bands[s];
        QColor fill = band.color;
        fill.setAlphaF(0.65);

        QPainterPath path;

        // Top edge: left to right with cubic bezier curves
        path.moveTo(baseX, midY - band.upper[0] * scale);
        for (int i = 1; i < numPoints; ++i) {
            qreal x = baseX + (static_cast<qreal>(i) / (numPoints - 1)) * plotW;
            qreal y = midY - band.upper[i] * scale;
            qreal prevX = baseX + (static_cast<qreal>(i - 1) / (numPoints - 1)) * plotW;
            qreal prevY = midY - band.upper[i - 1] * scale;
            qreal cpx = (prevX + x) / 2.0;
            path.cubicTo(cpx, prevY, cpx, y, x, y);
        }

        // Bottom edge: right to left
        for (int i = numPoints - 1; i >= 0; --i) {
            qreal x = baseX + (static_cast<qreal>(i) / (numPoints - 1)) * plotW;
            qreal y = midY - band.lower[i] * scale;
            if (i == numPoints - 1) {
                path.lineTo(x, y);
            } else {
                qreal nextX = baseX + (static_cast<qreal>(i + 1) / (numPoints - 1)) * plotW;
                qreal nextY = midY - band.lower[i + 1] * scale;
                qreal cpx = (nextX + x) / 2.0;
                path.cubicTo(cpx, nextY, cpx, y, x, y);
            }
        }
        path.closeSubpath();

        // Fill
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(path);

        // Outline
        p.setPen(QPen(band.color.darker(120), 0.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // Highlight peaks for dominant streams
        if (band.dominant) {
            qreal peakX = 0.0;
            qreal peakY = 0.0;
            qreal maxLocalH = 0.0;
            for (int i = 0; i < numPoints; ++i) {
                qreal localH = band.upper[i] - band.lower[i];
                if (localH > maxLocalH) {
                    maxLocalH = localH;
                    peakX = baseX + (static_cast<qreal>(i) / (numPoints - 1)) * plotW;
                    peakY = midY - band.upper[i] * scale;
                }
            }
            p.setPen(Qt::NoPen);
            p.setBrush(band.color);
            p.drawEllipse(QPointF(peakX, peakY), 4.0, 4.0);
            p.setBrush(Qt::white);
            p.drawEllipse(QPointF(peakX, peakY), 2.0, 2.0);
        }
    }

    // Center baseline
    p.setPen(QPen(QColor(100, 116, 139), 0.5, Qt::DashLine));
    p.drawLine(baseX, static_cast<int>(midY), baseX + plotW, static_cast<int>(midY));

    // Chart title
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x() + 5, rect.y() + 14, "Streamgraph");
}

void PaperStreamgraph::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Legend");

    auto counts = categoryCounts();
    const QStringList legendCats = {"Research", "Development", "Testing", "Deployment"};
    int itemH = qMin(28, (rect.height() - 50) / qMax(legendCats.size(), 1));

    for (int i = 0; i < legendCats.size(); ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(legendCats[i]) ? counts[legendCats[i]] : 0;
        QColor catColor = colorForCategory(legendCats[i]);

        // Color box
        p.setPen(Qt::NoPen);
        p.setBrush(catColor);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 2, 2);

        // Category name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, legendCats[i]);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " streams");
    }
}

void PaperStreamgraph::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };
    QList<Stat> stats = {
        {"Total Streams", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Value",     QString::number(avgValue(), 'f', 1), QColor(124, 58, 237)},
        {"Dominant",      QString::number(dominantCount()),    QColor(220, 38, 38)}
    };

    int boxH = qMin(42, (rect.height() - 10) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        // Background box
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperStreamgraph::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render a streamgraph");
        return;
    }
    infoLabel_->setText(QString("Streams: %1 | Avg: %2 | Dominant: %3")
        .arg(entries_.size())
        .arg(avgValue(), 0, 'f', 1)
        .arg(dominantCount()));
}

void PaperStreamgraph::loadSettings() {
    settings_.beginGroup("Streamgraph");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StreamgraphEntry e;
        e.id = settings_.value("id").toInt();
        e.stream = settings_.value("stream").toString();
        e.category = settings_.value("category").toString();
        e.period = settings_.value("period").toString();
        e.value = settings_.value("value").toDouble();
        e.peaks = settings_.value("peaks").toInt();
        e.dominant = settings_.value("dominant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperStreamgraph::saveSettings() {
    settings_.beginGroup("Streamgraph");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("stream", entries_[i].stream);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("peaks", entries_[i].peaks);
        settings_.setValue("dominant", entries_[i].dominant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
