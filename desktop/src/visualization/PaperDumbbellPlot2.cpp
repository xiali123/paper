#include "visualization/PaperDumbbellPlot2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDumbbellPlot2::PaperDumbbellPlot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DumbbellPlot2")
{
    setupUI();
    loadSettings();
}

void PaperDumbbellPlot2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperDumbbellPlot2::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Medical", "Financial", "Educational", "Environmental", "Technical"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter pair label...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDumbbellPlot2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render dumbbell plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 520);
}

void PaperDumbbellPlot2::addEntry(const DumbbellPlot2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pairSelected(entry.id, entry.rightVal - entry.leftVal);
    update();
}

QList<DumbbellPlot2Entry> PaperDumbbellPlot2::entries() const { return entries_; }

int PaperDumbbellPlot2::improvedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.improved) ++c;
    return c;
}

qreal PaperDumbbellPlot2::avgChange() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += (e.rightVal - e.leftVal);
    return sum / entries_.size();
}

QMap<QString, int> PaperDumbbellPlot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperDumbbellPlot2::onRender() {
    static const QStringList pairs = {"Before/After Treatment", "Pre/Post Study"};
    static const QStringList metrics = {"Accuracy", "F1-Score", "AUC", "Precision", "Recall"};
    static const QStringList categories = {"Medical", "Financial", "Educational", "Environmental", "Technical"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    entries_.clear();
    int cIdx = categoryCombo_->currentIndex();

    for (int i = 0; i < 8; ++i) {
        DumbbellPlot2Entry e;
        e.id = i + 1;
        e.pair = pairs[i % 2];
        e.metric = metrics[i % metrics.size()];
        e.category = (cIdx == 0)
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[cIdx - 1];
        e.leftVal = 40.0 + QRandomGenerator::global()->bounded(250) / 10.0;
        e.rightVal = 50.0 + QRandomGenerator::global()->bounded(450) / 10.0;
        e.improved = e.rightVal > e.leftVal;
        e.color = palette[i % 5];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty())
        emit pairSelected(entries_.last().id, entries_.last().rightVal - entries_.last().leftVal);
    update();
    inputField_->clear();
}

void PaperDumbbellPlot2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render dumbbell plot");
    update();
}

void PaperDumbbellPlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render dumbbell plot");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Dumbbell Plot 2 - Pair Comparison");

    int w = width(), h = height();
    drawDumbbellPlot(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDumbbellPlot2::drawDumbbellPlot(QPainter& p, const QRect& rect) {
    const int margin = 10;
    const int plotW = rect.width() - 2 * margin;
    const int plotH = rect.height() - 2 * margin;
    const int baseX = rect.x() + margin;
    const int baseY = rect.y() + margin;

    // Determine value range
    qreal maxVal = 0.0;
    for (const auto& e : entries_) {
        if (e.leftVal > maxVal) maxVal = e.leftVal;
        if (e.rightVal > maxVal) maxVal = e.rightVal;
    }
    maxVal = qMax(maxVal, 1.0);

    // Draw horizontal grid lines
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    const int gridSteps = 5;
    for (int g = 0; g <= gridSteps; ++g) {
        int gx = baseX + static_cast<int>((g / qreal(gridSteps)) * plotW);
        p.drawLine(gx, baseY, gx, baseY + plotH);
    }

    // Draw baseline axis
    p.setPen(QColor(203, 213, 225));
    p.drawLine(baseX, baseY + plotH, baseX + plotW, baseY + plotH);
    p.drawLine(baseX, baseY, baseX, baseY + plotH);

    // Axis value labels
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    for (int g = 0; g <= gridSteps; ++g) {
        qreal val = (g / qreal(gridSteps)) * maxVal;
        int gx = baseX + static_cast<int>((g / qreal(gridSteps)) * plotW);
        p.drawText(gx - 15, baseY + plotH + 2, 30, 12, Qt::AlignCenter,
                   QString::number(val, 'f', 0));
    }

    // Draw each row
    const int rowH = qMax(16, plotH / (entries_.size() + 1));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int y = baseY + 20 + i * rowH;
        int lx = baseX + static_cast<int>((e.leftVal / maxVal) * plotW);
        int rx = baseX + static_cast<int>((e.rightVal / maxVal) * plotW);

        // Connecting line colored by improved status
        QColor lineColor = e.improved ? QColor(22, 163, 74) : QColor(220, 38, 38);
        p.setPen(QPen(lineColor, 2));
        p.drawLine(lx, y, rx, y);

        // Left dot (gray)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(156, 163, 175));
        p.drawEllipse(lx - 5, y - 5, 10, 10);

        // Right dot (colored by entry color)
        p.setBrush(e.color);
        p.drawEllipse(rx - 5, y - 5, 10, 10);

        // Pair label on the left
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(baseX, y - 10, plotW, 12, Qt::AlignLeft,
                   e.pair + " - " + e.metric);

        // Value annotations near dots
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 6));
        p.drawText(lx - 12, y + 6, QString::number(e.leftVal, 'f', 1));
        p.drawText(rx - 12, y + 6, QString::number(e.rightVal, 'f', 1));
    }
}

void PaperDumbbellPlot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    static const QStringList categories = {"Medical", "Financial", "Educational", "Environmental", "Technical"};
    static const QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    auto counts = categoryCounts();
    const int itemH = qMin(28, (rect.height() - 60) / 5);

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, categories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " items");
    }

    // Improved indicator row
    int y = rect.y() + 22 + 5 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(22, 163, 74));
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Improved");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight,
               QString::number(improvedCount()));
}

void PaperDumbbellPlot2::drawStats(QPainter& p, const QRect& rect) {
    // Calculate max change
    qreal maxChange = 0.0;
    for (const auto& e : entries_) {
        qreal change = qAbs(e.rightVal - e.leftVal);
        if (change > maxChange) maxChange = change;
    }

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Pairs",     QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Improved Count",  QString::number(improvedCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Avg Change",      QString::number(avgChange(), 'f', 1), QColor(0xd9, 0x77, 0x06)},
        {"Max Change",      QString::number(maxChange, 'f', 1), QColor(0xdc, 0x26, 0x26)}
    };

    const int boxH = qMin(42, (rect.height() - 10) / 4);
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

void PaperDumbbellPlot2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render dumbbell plot");
        return;
    }
    infoLabel_->setText(
        QString("%1 pairs | %2 improved | avg change: %3")
            .arg(entries_.size())
            .arg(improvedCount())
            .arg(avgChange(), 0, 'f', 1));
}

void PaperDumbbellPlot2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DumbbellPlot2Entry e;
        e.id       = settings_.value("id").toInt();
        e.pair     = settings_.value("pair").toString();
        e.category = settings_.value("category").toString();
        e.metric   = settings_.value("metric").toString();
        e.leftVal  = settings_.value("leftVal").toDouble();
        e.rightVal = settings_.value("rightVal").toDouble();
        e.improved = settings_.value("improved").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDumbbellPlot2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("pair",     entries_[i].pair);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("metric",   entries_[i].metric);
        settings_.setValue("leftVal",  entries_[i].leftVal);
        settings_.setValue("rightVal", entries_[i].rightVal);
        settings_.setValue("improved", entries_[i].improved);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
