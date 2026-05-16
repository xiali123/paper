#include "visualization/PaperClevelandDotPlot.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperClevelandDotPlot::PaperClevelandDotPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ClevelandDotPlot")
{
    setupUI();
    loadSettings();
}

void PaperClevelandDotPlot::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Review", "Survey", "Case Study"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperClevelandDotPlot::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperClevelandDotPlot::onClear);
    toolbar->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Render a dot plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");

    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    setMinimumSize(700, 520);
}

void PaperClevelandDotPlot::addEntry(const ClevelandDotEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ClevelandDotEntry> PaperClevelandDotPlot::entries() const {
    return entries_;
}

int PaperClevelandDotPlot::leaderCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.leader) ++c;
    return c;
}

qreal PaperClevelandDotPlot::maxDiff() const {
    qreal mx = 0.0;
    for (const auto& e : entries_) {
        qreal d = qAbs(e.valueA - e.valueB);
        if (d > mx) mx = d;
    }
    return mx;
}

QMap<QString, int> PaperClevelandDotPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperClevelandDotPlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Research", "Review", "Survey", "Case Study"};
    QStringList groups = {"Group-A", "Group-B", "Group-C"};

    int cIdx = categoryCombo_->currentIndex();
    ClevelandDotEntry entry;
    entry.id = entries_.size() + 1;
    entry.label = text;
    entry.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
    entry.group = groups[QRandomGenerator::global()->bounded(groups.size())];
    entry.valueA = QRandomGenerator::global()->bounded(10001) / 100.0;
    entry.valueB = QRandomGenerator::global()->bounded(10001) / 100.0;
    entry.rank = entries_.size();
    entry.leader = entry.valueA > entry.valueB;

    QColor categoryColors[] = {
        QColor(59, 130, 246),  // #3b82f6 Research
        QColor(22, 163, 74),   // #16a34a Review
        QColor(217, 119, 6),   // #d97706 Survey
        QColor(220, 38, 38),   // #dc2626 Case Study
    };
    int ci = categories.indexOf(entry.category);
    entry.color = categoryColors[ci >= 0 ? ci : 0];

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dotSelected(entry.id, qAbs(entry.valueA - entry.valueB));
    update();
    inputField_->clear();
}

void PaperClevelandDotPlot::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperClevelandDotPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render a Cleveland dot plot");
        return;
    }

    int w = width();
    int h = height();

    // 3-column layout
    int col1W = w * 5 / 10;
    int col2W = w * 2 / 10;
    int col3W = w - col1W - col2W;
    int topY = 55;

    drawDumbbellChart(p, QRect(10, topY, col1W - 15, h - topY - 10));
    drawCategoryLegend(p, QRect(col1W + 5, topY, col2W - 10, h - topY - 10));
    drawStats(p, QRect(col1W + col2W + 5, topY, col3W - 10, h - topY - 10));
}

void PaperClevelandDotPlot::drawDumbbellChart(QPainter& p, const QRect& rect) {
    int margin = 15;
    int plotX = rect.x() + margin + 50; // room for labels
    int plotW = rect.width() - margin - 50 - margin;
    int plotY = rect.y() + 30;
    int plotH = rect.height() - 40;

    if (plotW < 50 || plotH < 50) return;

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 26, Qt::AlignLeft | Qt::AlignVCenter,
               "Cleveland Dot Plot");

    // X-axis scale: 0-100
    p.setPen(QColor(203, 213, 225));
    p.setFont(QFont("Arial", 7));
    for (int tick = 0; tick <= 100; tick += 20) {
        int tx = plotX + static_cast<int>((tick / 100.0) * plotW);
        p.drawLine(tx, plotY, tx, plotY + plotH);
        p.setPen(QColor(100, 116, 139));
        p.drawText(tx - 12, plotY + plotH + 2, 24, 14, Qt::AlignCenter, QString::number(tick));
        p.setPen(QColor(203, 213, 225));
    }

    // Baseline at bottom
    p.setPen(QColor(203, 213, 225));
    p.drawLine(plotX, plotY + plotH, plotX + plotW, plotY + plotH);

    int n = entries_.size();
    int rowH = qMax(14, qMin(28, plotH / (n + 1)));
    int dotR = qMax(3, qMin(7, rowH / 4));

    QColor blueColor(59, 130, 246);   // #3b82f6 valueA
    QColor orangeColor(217, 119, 6);  // #d97706 valueB

    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int cy = plotY + (i + 1) * (plotH / (n + 1));

        // Row label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        QString lbl = e.label.length() > 8 ? e.label.left(8) + ".." : e.label;
        p.drawText(rect.x() + margin, cy - 8, 48, 16, Qt::AlignRight | Qt::AlignVCenter, lbl);

        // Horizontal connecting line (dumbbell bar)
        int ax = plotX + static_cast<int>((e.valueA / 100.0) * plotW);
        int bx = plotX + static_cast<int>((e.valueB / 100.0) * plotW);
        int leftX = qMin(ax, bx);
        int rightX = qMax(ax, bx);

        p.setPen(QPen(QColor(203, 213, 225), 2));
        p.drawLine(leftX, cy, rightX, cy);

        // Dot A (blue)
        p.setPen(Qt::NoPen);
        p.setBrush(blueColor);
        p.drawEllipse(ax - dotR, cy - dotR, dotR * 2, dotR * 2);

        // Dot B (orange)
        p.setBrush(orangeColor);
        p.drawEllipse(bx - dotR, cy - dotR, dotR * 2, dotR * 2);

        // Value labels on hover-area (small text near dots)
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(ax - 12, cy - dotR - 10, 24, 10, Qt::AlignCenter,
                   QString::number(e.valueA, 'f', 1));
        p.drawText(bx - 12, cy - dotR - 10, 24, 10, Qt::AlignCenter,
                   QString::number(e.valueB, 'f', 1));
    }

    // Sub-legend for A vs B
    int legendY = plotY + plotH + 14;
    p.setFont(QFont("Arial", 8));
    p.setPen(Qt::NoPen);
    p.setBrush(blueColor);
    p.drawEllipse(plotX, legendY, 10, 10);
    p.setPen(QColor(71, 85, 105));
    p.drawText(plotX + 14, legendY + 1, 50, 10, Qt::AlignVCenter, "Value A");

    p.setPen(Qt::NoPen);
    p.setBrush(orangeColor);
    p.drawEllipse(plotX + 80, legendY, 10, 10);
    p.setPen(QColor(71, 85, 105));
    p.drawText(plotX + 94, legendY + 1, 50, 10, Qt::AlignVCenter, "Value B");
}

void PaperClevelandDotPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter, "Legend");

    auto counts = categoryCounts();
    QStringList categories = {"Research", "Review", "Survey", "Case Study"};
    QColor colors[] = {
        QColor(59, 130, 246),  // #3b82f6
        QColor(22, 163, 74),   // #16a34a
        QColor(217, 119, 6),   // #d97706
        QColor(220, 38, 38),   // #dc2626
    };

    int itemH = qMin(32, (rect.height() - 40) / (categories.size() + 2));
    int startY = rect.y() + 30;

    for (int i = 0; i < categories.size(); ++i) {
        int y = startY + i * (itemH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        // Colored square
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 2, 14, 14, 2, 2);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y, rect.width() - 60, 18, Qt::AlignVCenter, categories[i]);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() - 50, y, 46, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " pairs");
    }

    // Group section
    int groupY = startY + categories.size() * (itemH + 6) + 10;
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x() + 5, groupY, rect.width() - 10, 18, Qt::AlignVCenter, "Groups");

    QStringList groups = {"Group-A", "Group-B", "Group-C"};
    QColor groupColors[] = {
        QColor(124, 58, 237),  // #7c3aed
        QColor(59, 130, 246),  // #3b82f6
        QColor(22, 163, 74),   // #16a34a
    };
    QMap<QString, int> groupCounts;
    for (const auto& e : entries_) groupCounts[e.group]++;

    for (int i = 0; i < groups.size(); ++i) {
        int y = groupY + 20 + i * (itemH - 2);
        int count = groupCounts.contains(groups[i]) ? groupCounts[groups[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(groupColors[i]);
        p.drawEllipse(rect.x() + 7, y + 3, 10, 10);

        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 22, y, rect.width() - 30, 16, Qt::AlignVCenter,
                   groups[i] + " (" + QString::number(count) + ")");
    }
}

void PaperClevelandDotPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Pairs",   QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Leaders",       QString::number(leaderCount()),   QColor(22, 163, 74)},
        {"Max Diff",      QString::number(maxDiff(), 'f', 1), QColor(217, 119, 6)},
        {"Categories",    QString::number(categoryCounts().size()), QColor(220, 38, 38)},
        {"Avg Value A",
         entries_.isEmpty() ? "0.0"
             : QString::number(std::accumulate(entries_.begin(), entries_.end(), 0.0,
                   [](double s, const ClevelandDotEntry& e) { return s + e.valueA; })
                   / entries_.size(), 'f', 1),
         QColor(124, 58, 237)},
        {"Avg Value B",
         entries_.isEmpty() ? "0.0"
             : QString::number(std::accumulate(entries_.begin(), entries_.end(), 0.0,
                   [](double s, const ClevelandDotEntry& e) { return s + e.valueB; })
                   / entries_.size(), 'f', 1),
         QColor(59, 130, 246)},
    };

    int boxH = qMin(40, (rect.height() - 10) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 13, QFont::Bold));
        p.drawText(rect.x() + 8, y + 3, rect.width() - 16, 20, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 8, y + 22, rect.width() - 16, 14, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperClevelandDotPlot::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render a dot plot");
        return;
    }
    infoLabel_->setText(
        QString("Pairs: %1 | Leaders: %2 | Max Diff: %3")
            .arg(entries_.size())
            .arg(leaderCount())
            .arg(QString::number(maxDiff(), 'f', 1)));
}

void PaperClevelandDotPlot::loadSettings() {
    settings_.beginGroup("ClevelandDotPlot");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ClevelandDotEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group    = settings_.value("group").toString();
        e.valueA   = settings_.value("valueA").toDouble();
        e.valueB   = settings_.value("valueB").toDouble();
        e.rank     = settings_.value("rank").toInt();
        e.leader   = settings_.value("leader").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperClevelandDotPlot::saveSettings() {
    settings_.beginGroup("ClevelandDotPlot");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group",    entries_[i].group);
        settings_.setValue("valueA",   entries_[i].valueA);
        settings_.setValue("valueB",   entries_[i].valueB);
        settings_.setValue("rank",     entries_[i].rank);
        settings_.setValue("leader",   entries_[i].leader);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
