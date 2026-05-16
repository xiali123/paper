#include "visualization/PaperBoxPlotWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QRegularExpression>

PaperBoxPlotWidget::PaperBoxPlotWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BoxPlot")
{
    setupUI();
    loadSettings();
}

void PaperBoxPlotWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperBoxPlotWidget::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBoxPlotWidget::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Statistics", "Methods", "Results", "Discussion"});
    toolbar->addWidget(categoryCombo_, 1);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label;category;group;q1;median;q3 (e.g. Sample A;Statistics;Group-1;25;50;75)");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render box plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperBoxPlotWidget::addEntry(const BoxEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<BoxEntry> PaperBoxPlotWidget::entries() const {
    return entries_;
}

int PaperBoxPlotWidget::outlierCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.outlier) c++;
    return c;
}

qreal PaperBoxPlotWidget::maxMedian() const {
    if (entries_.isEmpty()) return 0.0;
    qreal mx = entries_.first().median;
    for (const auto& e : entries_)
        if (e.median > mx) mx = e.median;
    return mx;
}

QMap<QString, int> PaperBoxPlotWidget::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperBoxPlotWidget::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList parts = text.split(';');

    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6 blue
        QColor(22, 163, 74),    // #16a34a green
        QColor(217, 119, 6),    // #d97706 amber
        QColor(220, 38, 38),    // #dc2626 red
        QColor(124, 58, 237)    // #7c3aed purple
    };

    BoxEntry entry;
    entry.id = entries_.size() + 1;
    entry.label = parts.value(0, "Entry " + QString::number(entry.id));
    entry.category = parts.value(1, categoryCombo_->currentText() == "All"
        ? "Statistics" : categoryCombo_->currentText());
    entry.group = parts.value(2, "Group-1");

    if (parts.size() >= 6) {
        entry.q1 = parts.value(3, "25.0").toDouble();
        entry.median = parts.value(4, "50.0").toDouble();
        entry.q3 = parts.value(5, "75.0").toDouble();
    } else {
        entry.q1 = 20.0 + QRandomGenerator::global()->bounded(200) / 10.0;
        entry.median = entry.q1 + 10.0 + QRandomGenerator::global()->bounded(300) / 10.0;
        entry.q3 = entry.median + 10.0 + QRandomGenerator::global()->bounded(200) / 10.0;
    }

    entry.whiskerLow = entry.q1 - (5.0 + QRandomGenerator::global()->bounded(100) / 10.0);
    entry.whiskerHigh = entry.q3 + (5.0 + QRandomGenerator::global()->bounded(100) / 10.0);
    if (parts.size() >= 8) {
        entry.whiskerLow = parts.value(6, QString::number(entry.whiskerLow)).toDouble();
        entry.whiskerHigh = parts.value(7, QString::number(entry.whiskerHigh)).toDouble();
    }

    qreal iqr = entry.q3 - entry.q1;
    entry.outlier = (entry.whiskerHigh - entry.q3 > 1.5 * iqr) || (entry.q1 - entry.whiskerLow > 1.5 * iqr);

    int colorIdx = entry.id % 5;
    entry.color = entry.outlier ? QColor(220, 38, 38) : palette[colorIdx];

    addEntry(entry);
    emit boxRendered(entry.id, entry.median);
    inputField_->clear();
}

void PaperBoxPlotWidget::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render box plot");
    update();
}

void PaperBoxPlotWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render box plot");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Box Plot");

    int w = width(), h = height();
    drawBoxView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBoxPlotWidget::drawBoxView(QPainter& p, const QRect& area) {
    int margin = 15;
    int plotW = area.width() - 2 * margin;
    int plotH = area.height() - 2 * margin;
    if (plotW <= 0 || plotH <= 0) return;

    // Find global range for normalization
    qreal gMin = entries_.first().whiskerLow;
    qreal gMax = entries_.first().whiskerHigh;
    for (const auto& e : entries_) {
        if (e.whiskerLow < gMin) gMin = e.whiskerLow;
        if (e.whiskerHigh > gMax) gMax = e.whiskerHigh;
    }
    qreal range = gMax - gMin;
    if (range <= 0) range = 1.0;

    // Y-axis
    int axisX = area.x() + margin;
    int baseY = area.y() + margin + plotH;
    int topY = area.y() + margin;

    p.setPen(QColor(203, 213, 225));
    p.drawLine(axisX, topY, axisX, baseY);
    p.drawLine(axisX, baseY, area.x() + margin + plotW, baseY);

    // Y-axis labels
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    for (int tick = 0; tick <= 4; ++tick) {
        qreal val = gMin + (range * tick) / 4.0;
        int yPos = baseY - static_cast<int>((tick / 4.0) * plotH);
        p.drawText(axisX - 5, yPos - 8, 40, 16, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(val, 'f', 1));
        p.setPen(QColor(234, 236, 240));
        p.drawLine(axisX + 1, yPos, area.x() + margin + plotW, yPos);
        p.setPen(QColor(100, 116, 139));
    }

    // Draw boxes
    int n = entries_.size();
    int boxSpacing = qMax(4, plotW / (n * 3));
    int boxWidth = qMax(20, (plotW - (n + 1) * boxSpacing) / n);
    if (boxWidth > 60) boxWidth = 60;

    int totalBoxesWidth = n * boxWidth + (n + 1) * boxSpacing;
    int startX = axisX + (plotW - totalBoxesWidth) / 2 + boxSpacing;

    auto valToY = [&](qreal val) -> int {
        return baseY - static_cast<int>(((val - gMin) / range) * plotH);
    };

    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int cx = startX + i * (boxWidth + boxSpacing) + boxWidth / 2;
        int halfW = boxWidth / 2;

        int yQ1 = valToY(e.q1);
        int yMed = valToY(e.median);
        int yQ3 = valToY(e.q3);
        int yWL = valToY(e.whiskerLow);
        int yWH = valToY(e.whiskerHigh);

        // Whisker lines (vertical)
        p.setPen(QPen(QColor(100, 116, 139), 1));
        p.drawLine(cx, yQ1, cx, yWL);
        p.drawLine(cx, yQ3, cx, yWH);

        // Whisker caps (horizontal)
        int capW = qMax(6, boxWidth / 3);
        p.drawLine(cx - capW / 2, yWL, cx + capW / 2, yWL);
        p.drawLine(cx - capW / 2, yWH, cx + capW / 2, yWH);

        // Box (Q1 to Q3)
        QColor boxColor = e.color;
        p.setPen(Qt::NoPen);
        p.setBrush(boxColor.lighter(170));
        p.drawRoundedRect(cx - halfW, yQ3, boxWidth, yQ1 - yQ3, 3, 3);

        // Box border
        p.setPen(QPen(boxColor, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(cx - halfW, yQ3, boxWidth, yQ1 - yQ3, 3, 3);

        // Median line
        p.setPen(QPen(boxColor.darker(130), 2.5));
        p.drawLine(cx - halfW + 2, yMed, cx + halfW - 2, yMed);

        // Outlier marker
        if (e.outlier) {
            p.setPen(QPen(QColor(220, 38, 38), 2));
            p.setBrush(Qt::NoBrush);
            int markerY = yWH - 12;
            p.drawEllipse(cx - 5, markerY - 5, 10, 10);
        }

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        QString lbl = e.label.length() > 8 ? e.label.left(7) + ".." : e.label;
        p.drawText(cx - halfW, baseY + 4, boxWidth, 16, Qt::AlignCenter, lbl);
    }
}

void PaperBoxPlotWidget::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList knownCategories = {"Statistics", "Methods", "Results", "Discussion"};
    QColor palette[] = {
        QColor(59, 130, 246),   // blue
        QColor(22, 163, 74),    // green
        QColor(217, 119, 6),    // amber
        QColor(124, 58, 237),   // purple
        QColor(220, 38, 38)     // red
    };

    // Merge known with any unknown categories from data
    QStringList allCategories = knownCategories;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (!allCategories.contains(it.key()))
            allCategories.append(it.key());
    }

    int maxItems = qMin(allCategories.size(), 8);
    int itemH = qMin(28, (rect.height() - 50) / (maxItems + 1));

    for (int i = 0; i < maxItems; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(allCategories[i]) ? counts[allCategories[i]] : 0;
        QColor color = palette[i % 5];

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, allCategories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " entries");
    }

    // Outlier legend entry
    int y = rect.y() + 22 + maxItems * (itemH + 4);
    p.setPen(QPen(QColor(220, 38, 38), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(rect.x() + 7, y + 6, 10, 10);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, "Outliers");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(outlierCount()));
}

void PaperBoxPlotWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Outliers", QString::number(outlierCount()), QColor(220, 38, 38)},
        {"Max Median", QString::number(maxMedian(), 'f', 1), QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
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

void PaperBoxPlotWidget::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render box plot");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 outliers | max median: %3")
        .arg(entries_.size())
        .arg(outlierCount())
        .arg(QString::number(maxMedian(), 'f', 1)));
}

void PaperBoxPlotWidget::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BoxEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.q1 = settings_.value("q1").toDouble();
        e.median = settings_.value("median").toDouble();
        e.q3 = settings_.value("q3").toDouble();
        e.whiskerLow = settings_.value("whiskerLow").toDouble();
        e.whiskerHigh = settings_.value("whiskerHigh").toDouble();
        e.outlier = settings_.value("outlier").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBoxPlotWidget::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("q1", entries_[i].q1);
        settings_.setValue("median", entries_[i].median);
        settings_.setValue("q3", entries_[i].q3);
        settings_.setValue("whiskerLow", entries_[i].whiskerLow);
        settings_.setValue("whiskerHigh", entries_[i].whiskerHigh);
        settings_.setValue("outlier", entries_[i].outlier);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
