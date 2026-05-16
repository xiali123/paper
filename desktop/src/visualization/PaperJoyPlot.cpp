#include "visualization/PaperJoyPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <QPainterPath>

PaperJoyPlot::PaperJoyPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "JoyPlot")
{
    setupUI();
    loadSettings();
}

void PaperJoyPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperJoyPlot::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Results", "Discussion", "Background"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperJoyPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter series:value pairs (e.g. CNN:0.92, RNN:0.78, Transformer:0.95)");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render joy plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperJoyPlot::addEntry(const JoyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit joyRendered(entry.id, entry.peak);
    update();
}

QList<JoyEntry> PaperJoyPlot::entries() const { return entries_; }

int PaperJoyPlot::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dominant) c++;
    return c;
}

qreal PaperJoyPlot::maxPeak() const {
    qreal mp = 0;
    for (const auto& e : entries_) mp = qMax(mp, e.peak);
    return mp;
}

QMap<QString, int> PaperJoyPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperJoyPlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Methodology", "Results", "Discussion", "Background"};
    QStringList groups = {"deep-learning", "classical", "hybrid", "emerging"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),  // #3b82f6
        QColor(0x16, 0xa3, 0x4a),  // #16a34a
        QColor(0xd9, 0x77, 0x06),  // #d97706
        QColor(0xdc, 0x26, 0x26),  // #dc2626
        QColor(0x7c, 0x3a, 0xed)   // #7c3aed
    };

    entries_.clear();
    int cIdx = categoryCombo_->currentIndex();

    QStringList parts = text.split(',', Qt::SkipEmptyParts);
    int nextId = 1;
    for (const auto& part : parts) {
        QStringList kv = part.split(':', Qt::SkipEmptyParts);
        if (kv.size() < 2) continue;

        QString series = kv[0].trimmed();
        bool ok = false;
        qreal value = kv[1].trimmed().toDouble(&ok);
        if (!ok) continue;

        JoyEntry e;
        e.id = nextId++;
        e.series = series;
        e.category = cIdx == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];
        e.group = groups[QRandomGenerator::global()->bounded(groups.size())];
        e.value = qBound(0.0, value, 1.0);
        e.peak = 0.3 + QRandomGenerator::global()->bounded(700) / 1000.0;
        e.baseline = QRandomGenerator::global()->bounded(200) / 1000.0;
        e.dominant = e.value > 0.8;
        e.color = palette[(nextId - 2) % 5];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty()) {
        emit joyRendered(entries_.last().id, entries_.last().peak);
    }
    update();
    inputField_->clear();
}

void PaperJoyPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render joy plot");
    update();
}

void PaperJoyPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render joy plot");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Joy Plot (Ridgeline)");

    int w = width(), h = height();
    drawJoyView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperJoyPlot::drawJoyView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    if (n == 0) return;

    int marginX = 40;
    int marginTop = 10;
    int marginBottom = 30;
    int rowOverlap = static_cast<int>(rect.height() * 0.35 / qMax(1, n));
    int rowHeight = (rect.height() - marginTop - marginBottom) / qMax(1, n);
    int effectiveRowH = rowHeight + rowOverlap;
    int plotW = rect.width() - 2 * marginX;

    qreal globalMaxPeak = maxPeak();
    if (globalMaxPeak <= 0) globalMaxPeak = 1.0;

    // Group entries by series for ridgeline stacking
    QStringList seriesOrder;
    QMap<QString, QList<int>> seriesMap;
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        if (!seriesMap.contains(e.series)) {
            seriesOrder.append(e.series);
        }
        seriesMap[e.series].append(i);
    }

    int numRidges = seriesOrder.size();
    int ridgeSpacing = qMax(20, (rect.height() - marginTop - marginBottom) / qMax(1, numRidges));
    int overlapPx = static_cast<int>(ridgeSpacing * 0.45);

    for (int si = 0; si < numRidges; ++si) {
        const QString& series = seriesOrder[si];
        const QList<int>& indices = seriesMap[series];
        const JoyEntry& first = entries_[indices[0]];

        int baseY = rect.y() + marginTop + si * ridgeSpacing;
        int curveH = static_cast<int>((first.peak / globalMaxPeak) * ridgeSpacing * 1.8);

        // Build density curve as a series of points using Gaussian-like shape
        // centered around the value position
        int numPoints = 80;
        qreal centerX = first.value;

        QPainterPath curvePath;
        curvePath.moveTo(rect.x() + marginX, baseY);

        for (int pi = 0; pi <= numPoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numPoints;
            qreal xPos = t;

            // Gaussian density centered at centerX with spread based on group diversity
            qreal spread = 0.15 + (indices.size() - 1) * 0.03;
            qreal diff = xPos - centerX;
            qreal gauss = qExp(-(diff * diff) / (2.0 * spread * spread));
            // Secondary bump for multi-entry series
            qreal secondary = 0.0;
            if (indices.size() > 1) {
                qreal diff2 = xPos - (centerX + 0.2);
                secondary = 0.3 * qExp(-(diff2 * diff2) / (2.0 * 0.08 * 0.08));
            }

            qreal density = gauss + secondary;
            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(density * curveH);
            curvePath.lineTo(px, py);
        }

        curvePath.lineTo(rect.x() + marginX + plotW, baseY);
        curvePath.closeSubpath();

        // Fill with translucent color
        QColor fill(first.color.red(), first.color.green(), first.color.blue(), 100);
        QColor fillOpaque(first.color.red(), first.color.green(), first.color.blue(), 180);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(curvePath);

        // Stroke the curve outline
        p.setPen(QPen(first.dominant ? first.color : first.color.darker(120),
                       first.dominant ? 2.5 : 1.5));
        p.setBrush(Qt::NoBrush);
        QPainterPath strokePath;
        for (int pi = 0; pi <= numPoints; ++pi) {
            qreal t = static_cast<qreal>(pi) / numPoints;
            qreal xPos = t;
            qreal spread = 0.15 + (indices.size() - 1) * 0.03;
            qreal diff = xPos - centerX;
            qreal gauss = qExp(-(diff * diff) / (2.0 * spread * spread));
            qreal secondary = 0.0;
            if (indices.size() > 1) {
                qreal diff2 = xPos - (centerX + 0.2);
                secondary = 0.3 * qExp(-(diff2 * diff2) / (2.0 * 0.08 * 0.08));
            }
            qreal density = gauss + secondary;
            int px = rect.x() + marginX + static_cast<int>(t * plotW);
            int py = baseY - static_cast<int>(density * curveH);
            if (pi == 0) strokePath.moveTo(px, py);
            else strokePath.lineTo(px, py);
        }
        p.drawPath(strokePath);

        // Dominant marker
        if (first.dominant) {
            int markerX = rect.x() + marginX + static_cast<int>(centerX * plotW);
            int markerY = baseY - static_cast<int>(curveH * 0.85);
            p.setBrush(first.color);
            p.setPen(Qt::NoPen);
            p.drawEllipse(markerX - 4, markerY - 4, 8, 8);
        }

        // Series label on the left
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), baseY - 6, marginX - 4, 14,
                   Qt::AlignRight | Qt::AlignVCenter, first.series);
    }

    // Draw x-axis baseline
    if (numRidges > 0) {
        int lastY = rect.y() + marginTop + (numRidges - 1) * ridgeSpacing;
        p.setPen(QColor(203, 213, 225));
        p.drawLine(rect.x() + marginX, lastY + 4,
                   rect.x() + marginX + plotW, lastY + 4);

        // X-axis ticks
        p.setFont(QFont("Arial", 7));
        p.setPen(QColor(148, 163, 184));
        for (int tick = 0; tick <= 10; tick += 2) {
            qreal val = tick / 10.0;
            int tx = rect.x() + marginX + static_cast<int>(val * plotW);
            p.drawText(tx - 10, lastY + 8, 20, 14, Qt::AlignCenter,
                       QString::number(val, 'f', 1));
        }
    }
}

void PaperJoyPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Methodology", "Results", "Discussion", "Background"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26)
    };

    int itemH = qMin(28, (rect.height() - 50) / (categories.size() + 1));
    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 3, 14, 14, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " series");
    }

    // Dominant row
    int domY = rect.y() + 22 + categories.size() * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x7c, 0x3a, 0xed));
    p.drawEllipse(rect.x() + 7, domY + 5, 10, 10);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, domY + 2, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Dominant");

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, domY + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight,
               QString::number(dominantCount()));
}

void PaperJoyPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Series", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Dominant", QString::number(dominantCount()), QColor(0x7c, 0x3a, 0xed)},
        {"Max Peak", QString::number(maxPeak(), 'f', 2), QColor(0xd9, 0x77, 0x06)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x16, 0xa3, 0x4a)}
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

void PaperJoyPlot::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render joy plot");
        return;
    }
    infoLabel_->setText(QString("%1 series | %2 dominant | peak %3")
        .arg(entries_.size())
        .arg(dominantCount())
        .arg(maxPeak(), 0, 'f', 2));
}

void PaperJoyPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        JoyEntry e;
        e.id = settings_.value("id").toInt();
        e.series = settings_.value("series").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.value = settings_.value("value").toDouble();
        e.peak = settings_.value("peak").toDouble();
        e.baseline = settings_.value("baseline").toDouble();
        e.dominant = settings_.value("dominant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperJoyPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("series", entries_[i].series);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("peak", entries_[i].peak);
        settings_.setValue("baseline", entries_[i].baseline);
        settings_.setValue("dominant", entries_[i].dominant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
