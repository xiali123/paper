#include "visualization/PaperBeeswarmPlot2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <algorithm>
#include <numeric>

PaperBeeswarmPlot2::PaperBeeswarmPlot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BeeswarmPlot2")
{
    setupUI();
    loadSettings();
}

void PaperBeeswarmPlot2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    plotBtn_ = new QPushButton("Plot");
    plotBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(plotBtn_, &QPushButton::clicked, this, &PaperBeeswarmPlot2::onPlot);
    toolbar->addWidget(plotBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Biology", "Chemistry", "Physics", "Medicine", "Engineering"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBeeswarmPlot2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter beeswarm dataset or leave empty for seed data...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Plot beeswarm data");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(780, 560);
}

void PaperBeeswarmPlot2::addEntry(const BeeswarmPlot2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pointHovered(entry.id, entry.value);
    update();
}

QList<BeeswarmPlot2Entry> PaperBeeswarmPlot2::entries() const { return entries_; }

int PaperBeeswarmPlot2::extremeCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.extreme) ++c;
    return c;
}

qreal PaperBeeswarmPlot2::medianValue() const {
    if (entries_.isEmpty()) return 0.0;
    QList<qreal> vals;
    for (const auto& e : entries_) vals.append(e.value);
    std::sort(vals.begin(), vals.end());
    int n = vals.size();
    if (n % 2 == 0)
        return (vals[n / 2 - 1] + vals[n / 2]) / 2.0;
    return vals[n / 2];
}

QMap<QString, int> PaperBeeswarmPlot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBeeswarmPlot2::onPlot() {
    QString text = inputField_->text().trimmed();

    static const QStringList seriesNames = {"Gene A", "Gene B", "Gene C", "Gene D"};
    static const QStringList categories  = {"Biology", "Chemistry", "Physics", "Medicine", "Engineering"};
    static const QStringList groups      = {"Control", "Treatment", "Placebo"};
    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();

    if (text.isEmpty()) {
        // Seed 8 preset entries
        struct Seed { QString series; QString category; QString group; qreal value; int jitter; bool extreme; };
        Seed seeds[] = {
            {"Gene A", "Biology",     "Control",    23.5,  3,  false},
            {"Gene A", "Chemistry",   "Treatment",  48.2,  0,  true},
            {"Gene B", "Physics",     "Placebo",    71.3,  5,  false},
            {"Gene B", "Medicine",    "Control",    35.7,  2,  true},
            {"Gene C", "Engineering", "Treatment",  62.0,  7,  false},
            {"Gene C", "Biology",     "Placebo",    14.8,  1,  false},
            {"Gene D", "Chemistry",   "Control",    89.3,  4,  true},
            {"Gene D", "Physics",     "Treatment",  55.6,  6,  false},
        };
        for (int i = 0; i < 8; ++i) {
            BeeswarmPlot2Entry e;
            e.id       = i + 1;
            e.series   = seeds[i].series;
            e.category = seeds[i].category;
            e.group    = seeds[i].group;
            e.value    = seeds[i].value;
            e.jitter   = seeds[i].jitter;
            e.extreme  = seeds[i].extreme;
            int ci = categories.indexOf(e.category);
            e.color = palette[ci >= 0 ? ci % 5 : 0];
            if (cIdx > 0 && e.category != categories[cIdx - 1]) continue;
            entries_.append(e);
        }
    } else {
        // Generate random entries based on input text
        int count = 6 + QRandomGenerator::global()->bounded(10);
        for (int i = 0; i < count; ++i) {
            BeeswarmPlot2Entry e;
            e.id     = i + 1;
            e.series = seriesNames[QRandomGenerator::global()->bounded(seriesNames.size())];
            if (cIdx == 0)
                e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            else
                e.category = categories[cIdx - 1];
            e.group   = groups[QRandomGenerator::global()->bounded(groups.size())];
            e.value   = QRandomGenerator::global()->bounded(10000) / 10.0;
            e.jitter  = QRandomGenerator::global()->bounded(10);
            e.extreme = e.value > 85.0 || e.value < 10.0;
            int ci = categories.indexOf(e.category);
            e.color = palette[ci >= 0 ? ci % 5 : 0];
            entries_.append(e);
        }
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit pointHovered(entries_.last().id, entries_.last().value);
    update();
    inputField_->clear();
}

void PaperBeeswarmPlot2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Plot beeswarm data");
    update();
}

void PaperBeeswarmPlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Plot beeswarm data");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Beeswarm Plot 2");

    int w = width(), h = height();
    drawBeeswarm(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBeeswarmPlot2::drawBeeswarm(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;

    // Determine unique series in stable order
    static const QStringList seriesOrder = {"Gene A", "Gene B", "Gene C", "Gene D"};
    QSet<QString> seen;
    QStringList uniqueSeries;
    for (const auto& s : seriesOrder) {
        for (const auto& e : entries_) {
            if (e.series == s && !seen.contains(s)) {
                seen.insert(s);
                uniqueSeries.append(s);
                break;
            }
        }
    }
    for (const auto& e : entries_) {
        if (!seen.contains(e.series)) {
            seen.insert(e.series);
            uniqueSeries.append(e.series);
        }
    }

    int numSeries = uniqueSeries.size();
    if (numSeries == 0) return;

    // Compute global value range
    qreal minVal = entries_[0].value, maxVal = entries_[0].value;
    for (const auto& e : entries_) {
        if (e.value < minVal) minVal = e.value;
        if (e.value > maxVal) maxVal = e.value;
    }
    qreal valRange = maxVal - minVal;
    if (valRange <= 0) valRange = 100.0;

    // Axes
    int axisX = rect.x() + margin + 80;
    int axisY = rect.y() + margin;
    int axisBottom = rect.y() + margin + plotH;
    int axisRight = rect.x() + margin + plotW;

    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(axisX, axisY, axisX, axisBottom);
    p.drawLine(axisX, axisBottom, axisRight, axisBottom);

    // X-axis ticks (5 ticks)
    p.setFont(QFont("Arial", 7));
    for (int t = 0; t <= 4; ++t) {
        qreal val = minVal + valRange * t / 4.0;
        int xPos = axisX + static_cast<int>((t / 4.0) * (axisRight - axisX));
        p.setPen(QPen(QColor(203, 213, 225), 1));
        p.drawLine(xPos, axisBottom, xPos, axisBottom + 4);
        p.setPen(QColor(100, 116, 139));
        p.drawText(xPos - 20, axisBottom + 5, 40, 14, Qt::AlignCenter,
                   QString::number(val, 'f', 1));
    }

    int stripH = qMax(24, qMin(60, plotH / numSeries));
    int stripSpacing = qMax(6, (plotH - stripH * numSeries) / (numSeries + 1));

    for (int si = 0; si < numSeries; ++si) {
        const QString& seriesName = uniqueSeries[si];
        int stripTop = axisY + stripSpacing + si * (stripH + stripSpacing);
        int stripMid = stripTop + stripH / 2;

        // Light strip background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(axisX, stripTop, axisRight - axisX, stripH, 4, 4);

        // Series label on left
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + margin, stripTop, 76, stripH,
                   Qt::AlignRight | Qt::AlignVCenter, seriesName);

        // Collect values for this series to compute median
        QList<qreal> seriesVals;
        for (const auto& e : entries_) {
            if (e.series == seriesName)
                seriesVals.append(e.value);
        }
        std::sort(seriesVals.begin(), seriesVals.end());

        // Draw median dashed line
        if (!seriesVals.isEmpty()) {
            qreal medVal;
            int n = seriesVals.size();
            if (n % 2 == 0)
                medVal = (seriesVals[n / 2 - 1] + seriesVals[n / 2]) / 2.0;
            else
                medVal = seriesVals[n / 2];
            int medX = axisX + static_cast<int>(((medVal - minVal) / valRange) * (axisRight - axisX));
            medX = qBound(axisX, medX, axisRight);
            p.setPen(QPen(QColor(100, 116, 139), 2, Qt::DashLine));
            p.drawLine(medX, stripTop + 2, medX, stripTop + stripH - 2);
        }

        // Draw dots with y-jitter to avoid overlap (1D swarm)
        for (const auto& e : entries_) {
            if (e.series != seriesName) continue;
            int dotX = axisX + static_cast<int>(((e.value - minVal) / valRange) * (axisRight - axisX));
            dotX = qBound(axisX, dotX, axisRight);

            // Y-jitter based on entry jitter field to spread overlapping dots vertically
            int jitterOffset = ((e.jitter % 7) - 3) * (stripH / 10);
            int dotY = stripMid + jitterOffset;

            if (e.extreme) {
                // Extreme point: highlighted with outer ring
                p.setPen(QPen(QColor("#dc2626"), 2));
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(dotX - 8, dotY - 8, 16, 16);
                // Inner filled dot with category color
                p.setPen(Qt::NoPen);
                p.setBrush(e.color);
                p.drawEllipse(dotX - 4, dotY - 4, 8, 8);
            } else {
                // Normal dot colored by category
                p.setPen(Qt::NoPen);
                p.setBrush(e.color);
                p.drawEllipse(dotX - 5, dotY - 5, 10, 10);
            }
        }
    }
}

void PaperBeeswarmPlot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    static const QStringList categories = {"Biology", "Chemistry", "Physics", "Medicine", "Engineering"};
    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    auto counts = categoryCounts();
    int itemH = qMin(26, (rect.height() - 80) / 7);

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " pts");
    }

    // Extreme legend entry
    int y = rect.y() + 22 + 5 * (itemH + 4);
    p.setPen(QPen(QColor("#dc2626"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Extreme");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight,
               QString::number(extremeCount()));
}

void PaperBeeswarmPlot2::drawStats(QPainter& p, const QRect& rect) {
    qreal avgJitter = 0.0;
    if (!entries_.isEmpty()) {
        qreal sum = 0.0;
        for (const auto& e : entries_) sum += e.jitter;
        avgJitter = sum / entries_.size();
    }

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Points",    QString::number(entries_.size()),         QColor("#3b82f6")},
        {"Extreme Count",   QString::number(extremeCount()),          QColor("#dc2626")},
        {"Median Value",    QString::number(medianValue(), 'f', 1),   QColor("#d97706")},
        {"Avg Jitter",      QString::number(avgJitter, 'f', 1),       QColor("#7c3aed")},
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

void PaperBeeswarmPlot2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Plot beeswarm data");
        return;
    }
    infoLabel_->setText(QString("%1 points | %2 extreme | median %3")
        .arg(entries_.size())
        .arg(extremeCount())
        .arg(medianValue(), 0, 'f', 1));
}

void PaperBeeswarmPlot2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BeeswarmPlot2Entry e;
        e.id       = settings_.value("id").toInt();
        e.series   = settings_.value("series").toString();
        e.category = settings_.value("category").toString();
        e.group    = settings_.value("group").toString();
        e.value    = settings_.value("value").toDouble();
        e.jitter   = settings_.value("jitter").toInt();
        e.extreme  = settings_.value("extreme").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBeeswarmPlot2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("series",   entries_[i].series);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group",    entries_[i].group);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("jitter",   entries_[i].jitter);
        settings_.setValue("extreme",  entries_[i].extreme);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
