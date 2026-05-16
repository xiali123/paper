#include "visualization/PaperViolinChart2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QtMath>

PaperViolinChart2::PaperViolinChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ViolinChart2")
{
    setupUI();
    loadSettings();
}

void PaperViolinChart2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperViolinChart2::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citation", "Impact", "Quality", "Novelty", "Relevance"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("metric:median,samples  (e.g. h-index:42,120)");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperViolinChart2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Violin chart ready -- add entries or click Render");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(750, 550);
}

void PaperViolinChart2::addEntry(const ViolinChart2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ViolinChart2Entry> PaperViolinChart2::entries() const {
    return entries_;
}

int PaperViolinChart2::outlierCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.outlier) ++count;
    return count;
}

qreal PaperViolinChart2::avgMedian() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.median;
    return sum / entries_.size();
}

QMap<QString, int> PaperViolinChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperViolinChart2::onRender() {
    static const QStringList categories = {"Citation", "Impact", "Quality", "Novelty", "Relevance"};
    static const QStringList groups = {"group-A", "group-B", "group-C"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), // #3b82f6
        QColor(0x16, 0xa3, 0x4a), // #16a34a
        QColor(0xd9, 0x77, 0x06), // #d97706
        QColor(0xdc, 0x26, 0x26), // #dc2626
        QColor(0x7c, 0x3a, 0xed), // #7c3aed
    };

    // Try parsing manual input first
    QString text = inputField_->text().trimmed();
    if (!text.isEmpty()) {
        int colonPos = text.indexOf(':');
        if (colonPos > 0) {
            QString metric = text.left(colonPos).trimmed();
            QString dataPart = text.mid(colonPos + 1).trimmed();
            QStringList parts = dataPart.split(',');
            if (parts.size() >= 2) {
                bool okMed = false, okSamp = false;
                qreal median = parts[0].trimmed().toDouble(&okMed);
                int samples = parts[1].trimmed().toInt(&okSamp);
                if (okMed && okSamp) {
                    int catIdx = categoryCombo_->currentIndex();
                    QString category = catIdx == 0
                        ? categories[entries_.size() % categories.size()]
                        : categories[catIdx - 1];

                    ViolinChart2Entry e;
                    e.id = entries_.size() + 1;
                    e.metric = metric;
                    e.category = category;
                    e.group = groups[entries_.size() % groups.size()];
                    e.median = median;
                    e.samples = samples;
                    e.outlier = median > 80.0;
                    e.color = palette[categories.indexOf(category) % 5];
                    entries_.append(e);

                    saveSettings();
                    updateInfo();
                    emit violinSelected(e.id, e.median);
                    inputField_->clear();
                    update();
                    return;
                }
            }
        }
    }

    // Seed 8 random entries if no manual input
    int count = 8;
    for (int i = 0; i < count; ++i) {
        int catIdx = categoryCombo_->currentIndex();
        QString category = catIdx == 0
            ? categories[i % categories.size()]
            : categories[catIdx - 1];

        ViolinChart2Entry e;
        e.id = entries_.size() + 1;
        e.metric = category;
        e.category = category;
        e.group = groups[entries_.size() % groups.size()];
        e.median = 10.0 + QRandomGenerator::global()->bounded(900) / 10.0;
        e.samples = 20 + QRandomGenerator::global()->bounded(480);
        e.outlier = e.median > 80.0;
        e.color = palette[categories.indexOf(category) % 5];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit violinSelected(entries_.last().id, entries_.last().median);
    update();
}

void PaperViolinChart2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperViolinChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int margin = 10;
    int toolbarH = 60;
    int topY = toolbarH;
    int col1W = w * 60 / 100;
    int col2W = w * 20 / 100;
    int col3W = w - col1W - col2W - 4 * margin;

    drawViolinChart(p, QRect(margin, topY, col1W, h - topY - margin));
    drawCategoryLegend(p, QRect(2 * margin + col1W, topY, col2W, h - topY - margin));
    drawStats(p, QRect(3 * margin + col1W + col2W, topY, col3W, h - topY - margin));
}

void PaperViolinChart2::drawViolinChart(QPainter& p, const QRect& rect) {
    p.save();
    p.fillRect(rect, QColor(248, 250, 252));
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawRoundedRect(rect.adjusted(0, 0, -1, -1), 8, 8);

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    p.setPen(QColor(15, 23, 42));
    p.drawText(rect.adjusted(10, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Violin Distribution Chart");

    if (entries_.isEmpty()) {
        QFont hintFont = p.font();
        hintFont.setBold(false);
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect, Qt::AlignCenter, "No violin data -- add entries or click Render");
        p.restore();
        return;
    }

    // Group entries by category for side-by-side violins
    QMap<QString, QList<int>> catIndices;
    for (int i = 0; i < entries_.size(); ++i)
        catIndices[entries_[i].category].append(i);

    QStringList catKeys = catIndices.keys();
    int numGroups = catKeys.size();
    if (numGroups == 0) { p.restore(); return; }

    int titleH = 30;
    int marginLeft = 50;
    int marginBottom = 35;
    int marginTop = titleH + 10;
    QRect plotRect = rect.adjusted(marginLeft, marginTop, -10, -marginBottom);

    // Determine Y range from data
    qreal globalMin = 0.0;
    qreal globalMax = 100.0;
    for (const auto& e : entries_) {
        globalMax = qMax(globalMax, e.median * 1.3);
    }
    qreal range = globalMax - globalMin;
    if (range <= 0.0) range = 100.0;

    // Grid lines
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    int gridSteps = 5;
    for (int i = 0; i <= gridSteps; ++i) {
        qreal v = globalMin + (range * i) / gridSteps;
        int gy = plotRect.bottom() - static_cast<int>((v / range) * plotRect.height());
        p.drawLine(plotRect.left(), gy, plotRect.right(), gy);

        p.setPen(QColor(148, 163, 184));
        QFont tickFont;
        tickFont.setPointSize(8);
        p.setFont(tickFont);
        p.drawText(QRect(0, gy - 8, marginLeft - 6, 16),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(v, 'f', 0));
        p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    }

    // Y-axis
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(plotRect.left(), plotRect.top(), plotRect.left(), plotRect.bottom());
    // X-axis
    p.drawLine(plotRect.left(), plotRect.bottom(), plotRect.right(), plotRect.bottom());

    // Draw violin shapes per category group
    qreal groupWidth = static_cast<qreal>(plotRect.width()) / numGroups;

    for (int g = 0; g < numGroups; ++g) {
        const auto& indices = catIndices[catKeys[g]];
        int n = indices.size();

        qreal cx = plotRect.left() + g * groupWidth + groupWidth / 2.0;
        qreal maxHalfW = groupWidth * 0.38;

        // Compute density from samples spread across group
        // Each entry contributes a bell curve centered at its median
        // We build the mirrored density envelope
        int resolution = 60;
        QVector<qreal> density(resolution + 1, 0.0);

        // Build kernel density estimate
        qreal bandwidth = range * 0.12; // bandwidth for KDE
        for (int idx : indices) {
            const auto& e = entries_[idx];
            for (int r = 0; r <= resolution; ++r) {
                qreal v = globalMin + (range * r) / resolution;
                qreal diff = (v - e.median) / bandwidth;
                density[r] += qExp(-0.5 * diff * diff);
            }
        }

        // Normalize density to [0, 1]
        qreal maxDensity = 0.0;
        for (int r = 0; r <= resolution; ++r)
            maxDensity = qMax(maxDensity, density[r]);
        if (maxDensity > 0.0) {
            for (int r = 0; r <= resolution; ++r)
                density[r] /= maxDensity;
        }

        // Build the violin path: left side top-to-bottom, then right side bottom-to-top
        QPainterPath violinPath;
        QVector<QPointF> leftSide;
        QVector<QPointF> rightSide;

        for (int r = 0; r <= resolution; ++r) {
            qreal v = globalMin + (range * r) / resolution;
            int y = plotRect.bottom() - static_cast<int>((v / range) * plotRect.height());
            qreal hw = density[r] * maxHalfW;
            leftSide.append(QPointF(cx - hw, y));
            rightSide.append(QPointF(cx + hw, y));
        }

        violinPath.moveTo(leftSide[0]);
        for (int r = 1; r < leftSide.size(); ++r)
            violinPath.lineTo(leftSide[r]);
        for (int r = rightSide.size() - 1; r >= 0; --r)
            violinPath.lineTo(rightSide[r]);
        violinPath.closeSubpath();

        // Determine group color from first entry
        QColor groupColor = entries_[indices.first()].color;
        QColor fillColor = groupColor;
        fillColor.setAlpha(55);

        p.setPen(QPen(groupColor, 1.5));
        p.setBrush(fillColor);
        p.drawPath(violinPath);

        // Draw median markers for each entry in this group
        for (int idx : indices) {
            const auto& e = entries_[idx];
            qreal medY = plotRect.bottom() - (e.median / range) * plotRect.height();

            // Median dot
            p.setPen(Qt::NoPen);
            p.setBrush(e.outlier ? QColor("#dc2626") : Qt::white);
            p.drawEllipse(QPointF(cx, medY), 5, 5);

            p.setPen(e.outlier ? QColor("#dc2626") : groupColor.darker(130));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(cx, medY), 5, 5);

            // Outlier glow ring
            if (e.outlier) {
                QColor glowColor("#dc2626");
                glowColor.setAlpha(40);
                p.setPen(Qt::NoPen);
                p.setBrush(glowColor);
                p.drawEllipse(QPointF(cx, medY), 10, 10);
            }
        }

        // Category label on X-axis
        QFont labelFont;
        labelFont.setPointSize(8);
        p.setFont(labelFont);
        p.setPen(QColor(15, 23, 42));
        QString displayCat = catKeys[g].length() > 10 ? catKeys[g].left(9) + ".." : catKeys[g];
        p.drawText(QRectF(cx - groupWidth / 2, plotRect.bottom() + 4, groupWidth, 20),
                   Qt::AlignCenter, displayCat);
    }

    p.restore();
}

void PaperViolinChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.save();
    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Categories");

    static const QStringList categories = {"Citation", "Impact", "Quality", "Novelty", "Relevance"};
    static const QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6), // #3b82f6
        QColor(0x16, 0xa3, 0x4a), // #16a34a
        QColor(0xd9, 0x77, 0x06), // #d97706
        QColor(0xdc, 0x26, 0x26), // #dc2626
        QColor(0x7c, 0x3a, 0xed), // #7c3aed
    };

    auto counts = categoryCounts();

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    int y = rect.top() + 30;
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

    // Outlier legend
    y += 4;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#dc2626"));
    p.drawEllipse(rect.left() + 12, y + 2, 10, 10);
    p.setPen(QColor("#1e293b"));
    p.drawText(QRect(rect.left() + 30, y - 1, rect.width() - 40, 16),
               Qt::AlignVCenter | Qt::AlignLeft,
               "Outlier (" + QString::number(outlierCount()) + ")");

    p.restore();
}

void PaperViolinChart2::drawStats(QPainter& p, const QRect& rect) {
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
        {"Total Entries",  QString::number(entries_.size()),           QColor(0x3b, 0x82, 0xf6)},
        {"Outliers",       QString::number(outlierCount()),            QColor(0xdc, 0x26, 0x26)},
        {"Avg Median",     QString::number(avgMedian(), 'f', 1),      QColor(0xd9, 0x77, 0x06)},
        {"Categories",     QString::number(categoryCounts().size()),   QColor(0x7c, 0x3a, 0xed)}
    };

    int boxH = qMin(58, (rect.height() - 50) / 4);
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
        p.drawText(QRect(rect.left() + 10, y + 30, rect.width() - 20, 18),
                   Qt::AlignVCenter, stats[i].label);

        y += boxH + 8;
    }

    p.restore();
}

void PaperViolinChart2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Violin chart ready -- add entries or click Render");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | %2 outliers | avg median: %3 | %4 categories")
            .arg(entries_.size())
            .arg(outlierCount())
            .arg(avgMedian(), 0, 'f', 1)
            .arg(categoryCounts().size()));
}

void PaperViolinChart2::loadSettings() {
    settings_.beginGroup("ViolinChart2");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        ViolinChart2Entry e;
        e.id       = settings_.value("id").toInt();
        e.metric   = settings_.value("metric").toString();
        e.category = settings_.value("category").toString();
        e.group    = settings_.value("group").toString();
        e.median   = settings_.value("median").toDouble();
        e.samples  = settings_.value("samples").toInt();
        e.outlier  = settings_.value("outlier").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperViolinChart2::saveSettings() {
    settings_.beginGroup("ViolinChart2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",       e.id);
        settings_.setValue("metric",   e.metric);
        settings_.setValue("category", e.category);
        settings_.setValue("group",    e.group);
        settings_.setValue("median",   e.median);
        settings_.setValue("samples",  e.samples);
        settings_.setValue("outlier",  e.outlier);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
