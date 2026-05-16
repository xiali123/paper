#include "visualization/PaperViolinPlot2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperViolinPlot2::PaperViolinPlot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ViolinPlot2")
{
    setupUI();
    loadSettings();
}

void PaperViolinPlot2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* leftLayout = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Review", "Survey", "Case Study", "Meta Analysis"});
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftLayout->addWidget(inputField_);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperViolinPlot2::onRender);
    leftLayout->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperViolinPlot2::onClear);
    leftLayout->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Entries: 0 | Outliers: 0 | Max: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftLayout->addWidget(infoLabel_);

    mainLayout->addLayout(leftLayout);
    mainLayout->addStretch();

    setMinimumSize(700, 500);
}

void PaperViolinPlot2::onRender() {
    static const QStringList categories = {"Research", "Review", "Survey", "Case Study", "Meta Analysis"};
    static const QStringList colors = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};

    int count = 5 + QRandomGenerator::global()->bounded(6);

    for (int i = 0; i < count; ++i) {
        QString cat = categories[QRandomGenerator::global()->bounded(categories.size())];
        int colorIndex = categories.indexOf(cat) % colors.size();

        Violin2Entry entry;
        entry.id = entries_.size() + 1;
        entry.label = QString("V%1").arg(entry.id);
        entry.category = cat;
        entry.group = cat;
        entry.median = QRandomGenerator::global()->bounded(100);
        entry.spread = 5.0 + QRandomGenerator::global()->bounded(30);
        entry.outlier = entry.median > 90;
        entry.color = QColor(colors[colorIndex]);

        entries_.append(entry);
    }

    saveSettings();
    updateInfo();
    update();
}

void PaperViolinPlot2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperViolinPlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int margin = 10;
    int col1W = w * 60 / 100;
    int col2W = w * 20 / 100;
    int col3W = w - col1W - col2W - 4 * margin;

    int toolbarH = 50;
    int topY = toolbarH;
    int h = height() - topY - margin;

    drawViolinPlot(p, QRect(margin, topY, col1W, h));
    drawCategoryLegend(p, QRect(2 * margin + col1W, topY, col2W, h));
    drawStats(p, QRect(3 * margin + col1W + col2W, topY, col3W, h));
}

void PaperViolinPlot2::drawViolinPlot(QPainter& p, const QRect& rect) {
    p.save();

    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Violin Plot");

    int titleH = 28;
    QRect plotRect = rect.adjusted(40, titleH + 10, -10, -25);

    p.setPen(QPen(QColor("#e2e8f0"), 1, Qt::DotLine));
    for (int i = 0; i <= 5; ++i) {
        int gy = plotRect.top() + i * plotRect.height() / 5;
        p.drawLine(plotRect.left(), gy, plotRect.right(), gy);
    }

    QFont axisFont = p.font();
    axisFont.setPointSize(7);
    axisFont.setBold(false);
    p.setFont(axisFont);
    p.setPen(QColor("#64748b"));
    for (int i = 0; i <= 5; ++i) {
        int val = i * 20;
        int gy = plotRect.bottom() - i * plotRect.height() / 5;
        p.drawText(QRect(0, gy - 8, 36, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(val));
    }

    if (entries_.isEmpty()) {
        p.restore();
        return;
    }

    QMap<QString, QList<Violin2Entry>> grouped;
    for (const auto& e : entries_) {
        grouped[e.category].append(e);
    }

    QStringList keys = grouped.keys();
    int groupCount = keys.size();
    if (groupCount == 0) {
        p.restore();
        return;
    }

    qreal groupWidth = static_cast<qreal>(plotRect.width()) / groupCount;

    for (int g = 0; g < groupCount; ++g) {
        const auto& groupEntries = grouped[keys[g]];
        qreal cx = plotRect.left() + g * groupWidth + groupWidth / 2.0;

        qreal medians = 0.0;
        for (const auto& e : groupEntries) {
            medians += e.median;
        }
        qreal avgMedian = medians / groupEntries.size();

        qreal yCenter = plotRect.bottom() - (avgMedian / 100.0) * plotRect.height();

        qreal maxSpread = 0.0;
        for (const auto& e : groupEntries) {
            if (e.spread > maxSpread) maxSpread = e.spread;
        }
        qreal halfWidth = qMin(groupWidth * 0.4, maxSpread * plotRect.height() / 100.0 * 0.5);

        QColor fill = groupEntries.first().color;
        fill.setAlphaF(0.45);

        QPainterPath violin;
        violin.moveTo(cx, yCenter - halfWidth);
        violin.cubicTo(cx + halfWidth, yCenter - halfWidth * 0.3,
                       cx + halfWidth, yCenter + halfWidth * 0.3,
                       cx, yCenter + halfWidth);
        violin.cubicTo(cx - halfWidth, yCenter + halfWidth * 0.3,
                       cx - halfWidth, yCenter - halfWidth * 0.3,
                       cx, yCenter - halfWidth);
        violin.closeSubpath();

        p.setBrush(fill);
        p.setPen(QPen(groupEntries.first().color, 1.5));
        p.drawPath(violin);

        p.setPen(QPen(Qt::white, 2));
        qreal medianY = plotRect.bottom() - (avgMedian / 100.0) * plotRect.height();
        p.drawLine(QPointF(cx - halfWidth * 0.5, medianY), QPointF(cx + halfWidth * 0.5, medianY));

        for (const auto& e : groupEntries) {
            if (e.outlier) {
                qreal oy = plotRect.bottom() - (e.median / 100.0) * plotRect.height();
                p.setBrush(QColor("#dc2626"));
                p.setPen(Qt::NoPen);
                p.drawEllipse(QPointF(cx, oy), 3, 3);
            }
        }

        QFont labelFont;
        labelFont.setPointSize(7);
        p.setFont(labelFont);
        p.setPen(QColor("#334155"));
        p.drawText(QRectF(cx - groupWidth / 2, plotRect.bottom() + 4, groupWidth, 16),
                   Qt::AlignCenter, keys[g]);
    }

    p.restore();
}

void PaperViolinPlot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.save();

    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Legend");

    QMap<QString, int> counts = categoryCounts();
    static const QStringList categories = {"Research", "Review", "Survey", "Case Study", "Meta Analysis"};
    static const QStringList colorList = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    int y = rect.top() + 30;
    for (int i = 0; i < categories.size(); ++i) {
        QColor sqColor(colorList[i]);
        p.fillRect(QRect(rect.left() + 10, y, 14, 14), sqColor);
        p.setPen(QColor("#334155"));
        int cnt = counts.value(categories[i], 0);
        QString label = categories[i] + " (" + QString::number(cnt) + ")";
        p.drawText(QRect(rect.left() + 30, y - 1, rect.width() - 40, 16), Qt::AlignVCenter | Qt::AlignLeft, label);
        y += 22;
    }

    p.restore();
}

void PaperViolinPlot2::drawStats(QPainter& p, const QRect& rect) {
    p.save();

    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Statistics");

    QFont statFont = p.font();
    statFont.setBold(false);
    statFont.setPointSize(9);
    p.setFont(statFont);
    p.setPen(QColor("#334155"));

    int y = rect.top() + 32;

    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               "Total Entries:");
    p.drawText(QRect(rect.left() + 10, y + 18, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               QString::number(entries_.size()));

    y += 48;
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               "Outlier Count:");
    p.drawText(QRect(rect.left() + 10, y + 18, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               QString::number(outlierCount()));

    y += 48;
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               "Max Median:");
    p.drawText(QRect(rect.left() + 10, y + 18, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               QString::number(maxMedian(), 'f', 1));

    p.restore();
}

void PaperViolinPlot2::updateInfo() {
    QString info = QString("Entries: %1 | Outliers: %2 | Max: %3")
                       .arg(entries_.size())
                       .arg(outlierCount())
                       .arg(maxMedian(), 0, 'f', 1);
    infoLabel_->setText(info);
}

void PaperViolinPlot2::loadSettings() {
    settings_.beginGroup("ViolinPlot2");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        Violin2Entry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.median = settings_.value("median").toDouble();
        e.spread = settings_.value("spread").toDouble();
        e.outlier = settings_.value("outlier").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperViolinPlot2::saveSettings() {
    settings_.beginGroup("ViolinPlot2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("label", e.label);
        settings_.setValue("category", e.category);
        settings_.setValue("group", e.group);
        settings_.setValue("median", e.median);
        settings_.setValue("spread", e.spread);
        settings_.setValue("outlier", e.outlier);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperViolinPlot2::addEntry(const Violin2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<Violin2Entry> PaperViolinPlot2::entries() const {
    return entries_;
}

int PaperViolinPlot2::outlierCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.outlier) ++count;
    }
    return count;
}

qreal PaperViolinPlot2::maxMedian() const {
    qreal mv = 0.0;
    for (const auto& e : entries_) {
        if (e.median > mv) mv = e.median;
    }
    return mv;
}

QMap<QString, int> PaperViolinPlot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}
