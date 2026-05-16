#include "visualization/PaperBeeswarmChart2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <algorithm>
#include <numeric>

PaperBeeswarmChart2::PaperBeeswarmChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BeeswarmChart2")
{
    setupUI();
    loadSettings();
}

void PaperBeeswarmChart2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperBeeswarmChart2::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citation", "Impact", "Quality", "Novelty", "Relevance"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBeeswarmChart2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter beeswarm dataset or leave empty for seed data...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render beeswarm chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(780, 560);
}

void PaperBeeswarmChart2::addEntry(const BeeswarmChart2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pointSelected(entry.id, entry.value);
    update();
}

QList<BeeswarmChart2Entry> PaperBeeswarmChart2::entries() const { return entries_; }

int PaperBeeswarmChart2::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.highlighted) ++c;
    return c;
}

qreal PaperBeeswarmChart2::avgValue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.value;
    return sum / entries_.size();
}

QMap<QString, int> PaperBeeswarmChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBeeswarmChart2::onRender() {
    QString text = inputField_->text().trimmed();

    static const QStringList categories = {"Citation", "Impact", "Quality", "Novelty", "Relevance"};
    static const QStringList axes       = {"Score", "Index", "Rating", "Factor", "Metric"};
    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();

    if (text.isEmpty()) {
        // Seed 8 preset entries
        struct Seed { QString point; QString category; QString axis; qreal value; int cluster; bool highlighted; };
        Seed seeds[] = {
            {"P1",  "Citation",  "Score",   78.5,  3, true},
            {"P2",  "Impact",    "Index",   42.1,  1, false},
            {"P3",  "Quality",   "Rating",  91.7,  5, true},
            {"P4",  "Novelty",   "Factor",  63.4,  2, false},
            {"P5",  "Relevance", "Metric",  55.8,  4, false},
            {"P6",  "Citation",  "Score",   87.2,  3, true},
            {"P7",  "Impact",    "Index",   34.9,  1, false},
            {"P8",  "Quality",   "Rating",  70.6,  2, false},
        };
        for (int i = 0; i < 8; ++i) {
            BeeswarmChart2Entry e;
            e.id          = i + 1;
            e.point       = seeds[i].point;
            e.category    = seeds[i].category;
            e.axis        = seeds[i].axis;
            e.value       = seeds[i].value;
            e.cluster     = seeds[i].cluster;
            e.highlighted = seeds[i].highlighted;
            int ci = categories.indexOf(e.category);
            e.color = palette[ci >= 0 ? ci % 5 : 0];
            if (cIdx > 0 && e.category != categories[cIdx - 1]) continue;
            entries_.append(e);
        }
    } else {
        // Generate random entries based on input text
        int count = 6 + QRandomGenerator::global()->bounded(10);
        for (int i = 0; i < count; ++i) {
            BeeswarmChart2Entry e;
            e.id    = i + 1;
            e.point = QString("P%1").arg(i + 1);
            if (cIdx == 0)
                e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            else
                e.category = categories[cIdx - 1];
            e.axis        = axes[QRandomGenerator::global()->bounded(axes.size())];
            e.value       = QRandomGenerator::global()->bounded(10000) / 10.0;
            e.cluster     = 1 + QRandomGenerator::global()->bounded(5);
            e.highlighted = e.value > 85.0 || e.value < 10.0;
            int ci = categories.indexOf(e.category);
            e.color = palette[ci >= 0 ? ci % 5 : 0];
            entries_.append(e);
        }
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit pointSelected(entries_.last().id, entries_.last().value);
    update();
    inputField_->clear();
}

void PaperBeeswarmChart2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render beeswarm chart");
    update();
}

void PaperBeeswarmChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render beeswarm chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Beeswarm Chart 2");

    int w = width(), h = height();
    drawBeeswarm(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBeeswarmChart2::drawBeeswarm(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;

    // Determine unique categories in stable order
    static const QStringList catOrder = {"Citation", "Impact", "Quality", "Novelty", "Relevance"};
    QSet<QString> seen;
    QStringList uniqueCategories;
    for (const auto& c : catOrder) {
        for (const auto& e : entries_) {
            if (e.category == c && !seen.contains(c)) {
                seen.insert(c);
                uniqueCategories.append(c);
                break;
            }
        }
    }
    for (const auto& e : entries_) {
        if (!seen.contains(e.category)) {
            seen.insert(e.category);
            uniqueCategories.append(e.category);
        }
    }

    int numCats = uniqueCategories.size();
    if (numCats == 0) return;

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

    int stripH = qMax(24, qMin(60, plotH / numCats));
    int stripSpacing = qMax(6, (plotH - stripH * numCats) / (numCats + 1));

    // Collect placed dots per category for collision-avoidance jitter
    struct PlacedDot { int x; int y; int radius; };
    QMap<QString, QList<PlacedDot>> placed;

    for (int ci = 0; ci < numCats; ++ci) {
        const QString& catName = uniqueCategories[ci];
        int stripTop = axisY + stripSpacing + ci * (stripH + stripSpacing);
        int stripMid = stripTop + stripH / 2;

        // Light strip background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(axisX, stripTop, axisRight - axisX, stripH, 4, 4);

        // Category label on left
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + margin, stripTop, 76, stripH,
                   Qt::AlignRight | Qt::AlignVCenter, catName);

        // Compute average for this category (dashed line)
        QList<qreal> catVals;
        for (const auto& e : entries_) {
            if (e.category == catName)
                catVals.append(e.value);
        }
        if (!catVals.isEmpty()) {
            qreal avg = 0.0;
            for (qreal v : catVals) avg += v;
            avg /= catVals.size();
            int avgX = axisX + static_cast<int>(((avg - minVal) / valRange) * (axisRight - axisX));
            avgX = qBound(axisX, avgX, axisRight);
            p.setPen(QPen(QColor(100, 116, 139), 2, Qt::DashLine));
            p.drawLine(avgX, stripTop + 2, avgX, stripTop + stripH - 2);
        }

        // Draw dots with collision-avoidance jitter (beeswarm)
        for (const auto& e : entries_) {
            if (e.category != catName) continue;

            int dotX = axisX + static_cast<int>(((e.value - minVal) / valRange) * (axisRight - axisX));
            dotX = qBound(axisX, dotX, axisRight);

            // Dot size scaled by cluster (1-5 => 3-7 px radius)
            int radius = 3 + qBound(0, e.cluster - 1, 4);

            // Find jitter offset that avoids overlap with already-placed dots
            int bestY = stripMid;
            bool foundSpot = false;
            for (int yOff = 0; yOff <= stripH / 2; yOff += 2) {
                for (int sign : {0, -1, 1}) {
                    if (yOff == 0 && sign != 0) continue;
                    int candidateY = stripMid + sign * yOff;
                    if (candidateY - radius < stripTop || candidateY + radius > stripTop + stripH)
                        continue;
                    bool collision = false;
                    for (const auto& d : placed[catName]) {
                        int dx = dotX - d.x;
                        int dy = candidateY - d.y;
                        int minDist = radius + d.radius + 1;
                        if (dx * dx + dy * dy < minDist * minDist) {
                            collision = true;
                            break;
                        }
                    }
                    if (!collision) {
                        bestY = candidateY;
                        foundSpot = true;
                        break;
                    }
                }
                if (foundSpot) break;
            }

            placed[catName].append({dotX, bestY, radius});

            if (e.highlighted) {
                // Highlighted point: outer ring
                p.setPen(QPen(QColor("#dc2626"), 2));
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(dotX - radius - 3, bestY - radius - 3,
                              (radius + 3) * 2, (radius + 3) * 2);
            }

            // Filled dot colored by category
            p.setPen(Qt::NoPen);
            p.setBrush(e.color);
            p.drawEllipse(dotX - radius, bestY - radius, radius * 2, radius * 2);

            // Point label
            if (radius >= 5) {
                p.setPen(Qt::white);
                p.setFont(QFont("Arial", 6));
                p.drawText(dotX - radius, bestY - radius, radius * 2, radius * 2,
                           Qt::AlignCenter, e.point);
            }
        }
    }
}

void PaperBeeswarmChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    static const QStringList categories = {"Citation", "Impact", "Quality", "Novelty", "Relevance"};
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

    // Highlighted legend entry
    int y = rect.y() + 22 + 5 * (itemH + 4);
    p.setPen(QPen(QColor("#dc2626"), 2));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Highlighted");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight,
               QString::number(highlightedCount()));
}

void PaperBeeswarmChart2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Points",    QString::number(entries_.size()),        QColor("#3b82f6")},
        {"Highlighted",     QString::number(highlightedCount()),     QColor("#dc2626")},
        {"Avg Value",       QString::number(avgValue(), 'f', 1),    QColor("#d97706")},
        {"Categories",      QString::number(categoryCounts().size()), QColor("#7c3aed")},
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

void PaperBeeswarmChart2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render beeswarm chart");
        return;
    }
    infoLabel_->setText(QString("%1 points | %2 highlighted | avg %3")
        .arg(entries_.size())
        .arg(highlightedCount())
        .arg(avgValue(), 0, 'f', 1));
}

void PaperBeeswarmChart2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BeeswarmChart2Entry e;
        e.id          = settings_.value("id").toInt();
        e.point       = settings_.value("point").toString();
        e.category    = settings_.value("category").toString();
        e.axis        = settings_.value("axis").toString();
        e.value       = settings_.value("value").toDouble();
        e.cluster     = settings_.value("cluster").toInt();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBeeswarmChart2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",          entries_[i].id);
        settings_.setValue("point",       entries_[i].point);
        settings_.setValue("category",    entries_[i].category);
        settings_.setValue("axis",        entries_[i].axis);
        settings_.setValue("value",       entries_[i].value);
        settings_.setValue("cluster",     entries_[i].cluster);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color",       entries_[i].color.name());
    }
    settings_.endArray();
}
