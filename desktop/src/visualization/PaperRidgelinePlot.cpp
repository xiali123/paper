#include "visualization/PaperRidgelinePlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <QPainterPath>

PaperRidgelinePlot::PaperRidgelinePlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RidgelinePlot")
{
    setupUI();
    loadSettings();
}

void PaperRidgelinePlot::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Distribution", "Frequency", "Density", "Trend"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperRidgelinePlot::onRender);
    leftPanel->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRidgelinePlot::onClear);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Ridges: 0 | Peaks: 0 | Max Density: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();

    mainLayout->addLayout(leftPanel, 0);
    mainLayout->addStretch(1);

    setMinimumSize(600, 500);
}

void PaperRidgelinePlot::addEntry(const RidgelineEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<RidgelineEntry> PaperRidgelinePlot::entries() const { return entries_; }

int PaperRidgelinePlot::peakCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.peak) c++;
    return c;
}

qreal PaperRidgelinePlot::maxDensity() const {
    qreal md = 0;
    for (const auto& e : entries_) md = qMax(md, e.density);
    return md;
}

QMap<QString, int> PaperRidgelinePlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRidgelinePlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Distribution", "Frequency", "Density", "Trend"};
    QStringList groups = {"group-A", "group-B", "group-C", "group-D"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),  // #3b82f6
        QColor(0x16, 0xa3, 0x4a),  // #16a34a
        QColor(0xd9, 0x77, 0x06),  // #d97706
        QColor(0xdc, 0x26, 0x26),  // #dc2626
        QColor(0x7c, 0x3a, 0xed)   // #7c3aed
    };

    int cIdx = categoryCombo_->currentIndex();

    RidgelineEntry e;
    e.id = entries_.size() + 1;
    e.label = text;
    e.category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    e.group = groups[QRandomGenerator::global()->bounded(groups.size())];
    e.value = QRandomGenerator::global()->bounded(101);
    e.density = QRandomGenerator::global()->bounded(1001) / 1000.0;
    e.rank = entries_.size();
    e.peak = e.density > 0.8;
    e.color = palette[entries_.size() % 5];
    entries_.append(e);

    saveSettings();
    updateInfo();
    emit ridgeSelected(e.id, e.density);
    update();
    inputField_->clear();
}

void PaperRidgelinePlot::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperRidgelinePlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render ridgeline plot");
        return;
    }

    int w = width(), h = height();
    int colW = w / 3;
    drawRidgelines(p, QRect(10, 10, colW - 10, h - 20));
    drawCategoryLegend(p, QRect(colW + 5, 10, colW - 10, h - 20));
    drawStats(p, QRect(2 * colW + 5, 10, colW - 15, h - 20));
}

void PaperRidgelinePlot::drawRidgelines(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 28, Qt::AlignLeft | Qt::AlignTop, "Ridgeline Plot");

    int n = entries_.size();
    if (n == 0) return;

    int marginTop = 40;
    int marginBottom = 30;
    int marginX = 50;
    int plotW = rect.width() - 2 * marginX;
    int plotH = rect.height() - marginTop - marginBottom;

    int rowSpacing = qMax(20, plotH / qMax(1, n));
    int overlapPx = static_cast<int>(rowSpacing * 0.45);

    qreal globalMaxDensity = maxDensity();
    if (globalMaxDensity <= 0) globalMaxDensity = 1.0;

    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];
        int baseY = rect.y() + marginTop + i * rowSpacing;
        int curveH = static_cast<int>((entry.density / globalMaxDensity) * rowSpacing * 1.8);

        int numPoints = 80;
        qreal centerX = entry.value / 100.0;

        // Build filled wave path
        QPainterPath fillPath;
        fillPath.moveTo(rect.x() + marginX, baseY);

        for (int pi = 0; pi <= numPoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numPoints;
            qreal spread = 0.12;
            qreal diff = t - centerX;
            qreal gauss = qExp(-(diff * diff) / (2.0 * spread * spread));

            // Add a secondary bump for visual interest when density is high
            qreal secondary = 0.0;
            if (entry.density > 0.5) {
                qreal diff2 = t - (centerX + 0.15);
                secondary = 0.25 * entry.density * qExp(-(diff2 * diff2) / (2.0 * 0.06 * 0.06));
            }

            qreal yVal = gauss + secondary;
            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(yVal * curveH);
            fillPath.lineTo(px, py);
        }

        fillPath.lineTo(rect.x() + marginX + plotW, baseY);
        fillPath.closeSubpath();

        // Fill with 40% alpha color
        QColor fill(entry.color.red(), entry.color.green(), entry.color.blue(), 102);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(fillPath);

        // Stroke outline
        QPainterPath strokePath;
        for (int pi = 0; pi <= numPoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numPoints;
            qreal spread = 0.12;
            qreal diff = t - centerX;
            qreal gauss = qExp(-(diff * diff) / (2.0 * spread * spread));

            qreal secondary = 0.0;
            if (entry.density > 0.5) {
                qreal diff2 = t - (centerX + 0.15);
                secondary = 0.25 * entry.density * qExp(-(diff2 * diff2) / (2.0 * 0.06 * 0.06));
            }

            qreal yVal = gauss + secondary;
            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(yVal * curveH);
            if (pi == 0) strokePath.moveTo(px, py);
            else strokePath.lineTo(px, py);
        }
        p.setPen(QPen(entry.peak ? entry.color : entry.color.darker(120),
                       entry.peak ? 2.5 : 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(strokePath);

        // Peak highlight marker
        if (entry.peak) {
            int markerX = rect.x() + marginX + static_cast<int>(centerX * plotW);
            int markerY = baseY - static_cast<int>(curveH * 0.85);
            p.setBrush(entry.color);
            p.setPen(Qt::NoPen);
            p.drawEllipse(markerX - 4, markerY - 4, 8, 8);
        }

        // Label on the left
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), baseY - 6, marginX - 4, 14,
                   Qt::AlignRight | Qt::AlignVCenter, entry.label);
    }

    // X-axis baseline
    if (n > 0) {
        int lastY = rect.y() + marginTop + (n - 1) * rowSpacing;
        p.setPen(QColor(203, 213, 225));
        p.drawLine(rect.x() + marginX, lastY + 4,
                   rect.x() + marginX + plotW, lastY + 4);

        // X-axis ticks
        p.setFont(QFont("Arial", 7));
        p.setPen(QColor(148, 163, 184));
        for (int tick = 0; tick <= 100; tick += 20) {
            qreal val = tick / 100.0;
            int tx = rect.x() + marginX + static_cast<int>(val * plotW);
            p.drawText(tx - 10, lastY + 8, 20, 14, Qt::AlignCenter,
                       QString::number(tick));
        }
    }
}

void PaperRidgelinePlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 28, Qt::AlignLeft | Qt::AlignTop, "Legend");

    auto counts = categoryCounts();
    QStringList categories = {"Distribution", "Frequency", "Density", "Trend"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26)
    };

    int itemH = qMin(28, (rect.height() - 80) / (categories.size() + 1));
    int startY = rect.y() + 38;

    for (int i = 0; i < categories.size(); ++i) {
        int y = startY + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        // Colored square
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 3, 14, 14, 3, 3);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i]);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " ridges");
    }

    // Peak indicator row
    int peakY = startY + categories.size() * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x7c, 0x3a, 0xed));
    p.drawEllipse(rect.x() + 7, peakY + 5, 10, 10);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, peakY + 2, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Peak");

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, peakY + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight,
               QString::number(peakCount()));
}

void PaperRidgelinePlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Ridges", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Peak Count", QString::number(peakCount()), QColor(0x7c, 0x3a, 0xed)},
        {"Max Density", QString::number(maxDensity(), 'f', 2), QColor(0xd9, 0x77, 0x06)}
    };

    int boxH = qMin(60, (rect.height() - 10) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 8);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 28,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 34, rect.width() - 20, 18,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperRidgelinePlot::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Ridges: 0 | Peaks: 0 | Max Density: 0.00");
        return;
    }
    infoLabel_->setText(QString("Ridges: %1 | Peaks: %2 | Max Density: %3")
        .arg(entries_.size())
        .arg(peakCount())
        .arg(maxDensity(), 0, 'f', 2));
}

void PaperRidgelinePlot::loadSettings() {
    settings_.beginGroup("RidgelinePlot");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RidgelineEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.value = settings_.value("value").toDouble();
        e.density = settings_.value("density").toDouble();
        e.rank = settings_.value("rank").toInt();
        e.peak = settings_.value("peak").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperRidgelinePlot::saveSettings() {
    settings_.beginGroup("RidgelinePlot");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("density", entries_[i].density);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("peak", entries_[i].peak);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
