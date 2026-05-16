#include "visualization/PaperRidgelineChart2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

PaperRidgelineChart2::PaperRidgelineChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RidgelineChart2")
{
    setupUI();
    loadSettings();
}

void PaperRidgelineChart2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperRidgelineChart2::onRender);
    toolbar->addWidget(renderBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Distribution", "Frequency", "Density", "Trend"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRidgelineChart2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);
    mainLayout->addStretch();

    infoLabel_ = new QLabel("Ridges: 0 | Outliers: 0 | Max Peak: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(700, 500);
}

void PaperRidgelineChart2::addEntry(const Ridge2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<Ridge2Entry> PaperRidgelineChart2::entries() const {
    return entries_;
}

int PaperRidgelineChart2::outlierCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.outlier) ++count;
    }
    return count;
}

qreal PaperRidgelineChart2::maxPeak() const {
    qreal mp = 0.0;
    for (const auto& e : entries_) {
        if (e.peak > mp) mp = e.peak;
    }
    return mp;
}

QMap<QString, int> PaperRidgelineChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperRidgelineChart2::onRender() {
    static const QStringList categories = {"Distribution", "Frequency", "Density", "Trend"};
    static const QStringList groups = {"group-A", "group-B", "group-C", "group-D"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int count = 5 + QRandomGenerator::global()->bounded(6);

    for (int i = 0; i < count; ++i) {
        Ridge2Entry e;
        e.id = entries_.size() + 1;
        e.label = QString("Ridge %1").arg(e.id);
        int cIdx = categoryCombo_->currentIndex();
        e.category = cIdx == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];
        e.group = groups[QRandomGenerator::global()->bounded(groups.size())];
        e.peak = QRandomGenerator::global()->bounded(1001) / 1000.0;
        e.spread = 0.05 + QRandomGenerator::global()->bounded(200) / 1000.0;
        e.outlier = e.peak > 0.85;
        e.color = palette[entries_.size() % 5];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty()) {
        emit ridgeSelected(entries_.last().id, entries_.last().peak);
    }
    update();
}

void PaperRidgelineChart2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperRidgelineChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int toolbarH = 50;
    int topY = toolbarH;
    int margin = 10;
    int colW = (w - 4 * margin) / 3;

    drawRidgeline(p, QRect(margin, topY, colW, h - topY - margin));
    drawCategoryLegend(p, QRect(2 * margin + colW, topY, colW, h - topY - margin));
    drawStats(p, QRect(3 * margin + 2 * colW, topY, colW, h - topY - margin));
}

void PaperRidgelineChart2::drawRidgeline(QPainter& p, const QRect& rect) {
    p.save();
    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Ridgeline Chart");

    if (entries_.isEmpty()) {
        QFont hintFont = p.font();
        hintFont.setBold(false);
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect, Qt::AlignCenter, "No data - click Render");
        p.restore();
        return;
    }

    int marginTop = 38;
    int marginBottom = 28;
    int marginX = 60;
    int plotW = rect.width() - 2 * marginX;
    int plotH = rect.height() - marginTop - marginBottom;

    int n = entries_.size();
    int rowSpacing = qMax(18, plotH / qMax(1, n));

    qreal globalMax = maxPeak();
    if (globalMax <= 0) globalMax = 1.0;

    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];
        int baseY = rect.y() + marginTop + i * rowSpacing;
        int curveH = static_cast<int>((entry.peak / globalMax) * rowSpacing * 1.8);

        int numPoints = 80;
        qreal centerX = 0.2 + (i % 5) * 0.15;

        QPainterPath fillPath;
        fillPath.moveTo(rect.x() + marginX, baseY);

        for (int pi = 0; pi <= numPoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numPoints;
            qreal diff = t - centerX;
            qreal gauss = qExp(-(diff * diff) / (2.0 * entry.spread * entry.spread));

            qreal secondary = 0.0;
            if (entry.peak > 0.5) {
                qreal diff2 = t - (centerX + 0.12);
                secondary = 0.2 * entry.peak * qExp(-(diff2 * diff2) / (2.0 * 0.05 * 0.05));
            }

            qreal yVal = gauss + secondary;
            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(yVal * curveH);
            fillPath.lineTo(px, py);
        }

        fillPath.lineTo(rect.x() + marginX + plotW, baseY);
        fillPath.closeSubpath();

        QColor fill(entry.color.red(), entry.color.green(), entry.color.blue(), 90);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(fillPath);

        QPainterPath strokePath;
        for (int pi = 0; pi <= numPoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numPoints;
            qreal diff = t - centerX;
            qreal gauss = qExp(-(diff * diff) / (2.0 * entry.spread * entry.spread));

            qreal secondary = 0.0;
            if (entry.peak > 0.5) {
                qreal diff2 = t - (centerX + 0.12);
                secondary = 0.2 * entry.peak * qExp(-(diff2 * diff2) / (2.0 * 0.05 * 0.05));
            }

            qreal yVal = gauss + secondary;
            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(yVal * curveH);
            if (pi == 0) strokePath.moveTo(px, py);
            else strokePath.lineTo(px, py);
        }

        p.setPen(QPen(entry.outlier ? QColor("#dc2626") : entry.color,
                       entry.outlier ? 2.5 : 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(strokePath);

        if (entry.outlier) {
            int markerX = rect.x() + marginX + static_cast<int>(centerX * plotW);
            int markerY = baseY - static_cast<int>(curveH * 0.85);
            p.setBrush(QColor("#dc2626"));
            p.setPen(Qt::NoPen);
            p.drawEllipse(markerX - 4, markerY - 4, 8, 8);
        }

        p.setPen(QColor("#1e293b"));
        QFont labelFont;
        labelFont.setPointSize(7);
        p.setFont(labelFont);
        p.drawText(rect.x(), baseY - 6, marginX - 4, 14,
                   Qt::AlignRight | Qt::AlignVCenter, entry.label);
    }

    if (n > 0) {
        int lastY = rect.y() + marginTop + (n - 1) * rowSpacing;
        p.setPen(QColor("#cbd5e1"));
        p.drawLine(rect.x() + marginX, lastY + 4,
                   rect.x() + marginX + plotW, lastY + 4);

        QFont tickFont;
        tickFont.setPointSize(7);
        p.setFont(tickFont);
        p.setPen(QColor("#94a3b8"));
        for (int tick = 0; tick <= 100; tick += 20) {
            qreal val = tick / 100.0;
            int tx = rect.x() + marginX + static_cast<int>(val * plotW);
            p.drawText(tx - 10, lastY + 8, 20, 14, Qt::AlignCenter,
                       QString::number(tick));
        }
    }

    p.restore();
}

void PaperRidgelineChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.save();
    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Legend");

    static const QStringList categories = {"Distribution", "Frequency", "Density", "Trend"};
    static const QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26)
    };

    auto counts = categoryCounts();
    int y = rect.top() + 30;

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    for (int i = 0; i < categories.size(); ++i) {
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(rect.left() + 10, y, 14, 14, 3, 3);

        p.setPen(QColor("#1e293b"));
        p.setFont(itemFont);
        int cnt = counts.value(categories[i], 0);
        p.drawText(QRect(rect.left() + 30, y - 1, rect.width() - 40, 16),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   categories[i] + " (" + QString::number(cnt) + ")");
        y += 24;
    }

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x7c, 0x3a, 0xed));
    p.drawEllipse(rect.left() + 11, y + 2, 10, 10);

    p.setPen(QColor("#1e293b"));
    p.drawText(QRect(rect.left() + 30, y - 1, rect.width() - 40, 16),
               Qt::AlignVCenter | Qt::AlignLeft,
               "Outlier (" + QString::number(outlierCount()) + ")");

    p.restore();
}

void PaperRidgelineChart2::drawStats(QPainter& p, const QRect& rect) {
    p.save();
    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Statistics");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Ridges", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Outliers", QString::number(outlierCount()), QColor(0xdc, 0x26, 0x26)},
        {"Max Peak", QString::number(maxPeak(), 'f', 2), QColor(0x7c, 0x3a, 0xed)}
    };

    int boxH = qMin(60, (rect.height() - 50) / 3);
    int y = rect.top() + 30;

    for (int i = 0; i < stats.size(); ++i) {
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.left() + 5, y, rect.width() - 10, boxH, 6, 6);

        p.setPen(stats[i].color);
        QFont valFont;
        valFont.setPointSize(14);
        valFont.setBold(true);
        p.setFont(valFont);
        p.drawText(QRect(rect.left() + 10, y + 4, rect.width() - 20, 26),
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor("#64748b"));
        QFont labelFont;
        labelFont.setPointSize(8);
        labelFont.setBold(false);
        p.setFont(labelFont);
        p.drawText(QRect(rect.left() + 10, y + 32, rect.width() - 20, 18),
                   Qt::AlignVCenter, stats[i].label);

        y += boxH + 8;
    }

    p.restore();
}

void PaperRidgelineChart2::updateInfo() {
    QString info = QString("Ridges: %1 | Outliers: %2 | Max Peak: %3")
                       .arg(entries_.size())
                       .arg(outlierCount())
                       .arg(maxPeak(), 0, 'f', 2);
    infoLabel_->setText(info);
}

void PaperRidgelineChart2::loadSettings() {
    settings_.beginGroup("RidgelineChart2");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        Ridge2Entry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.peak = settings_.value("peak").toDouble();
        e.spread = settings_.value("spread").toDouble();
        e.outlier = settings_.value("outlier").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperRidgelineChart2::saveSettings() {
    settings_.beginGroup("RidgelineChart2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("label", e.label);
        settings_.setValue("category", e.category);
        settings_.setValue("group", e.group);
        settings_.setValue("peak", e.peak);
        settings_.setValue("spread", e.spread);
        settings_.setValue("outlier", e.outlier);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
