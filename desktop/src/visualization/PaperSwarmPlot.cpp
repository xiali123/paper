#include "visualization/PaperSwarmPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSwarmPlot::PaperSwarmPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SwarmPlot")
{
    setupUI();
    loadSettings();
}

void PaperSwarmPlot::setupUI() {
    auto* layout = new QHBoxLayout(this);

    // Left panel: controls
    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Category-A", "Category-B", "Category-C", "Category-D", "Category-E"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperSwarmPlot::onRender);
    leftPanel->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSwarmPlot::onClear);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Points: 0 | Outliers: 0 | Max: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();
    layout->addLayout(leftPanel);

    // Right area reserved for custom painting (paintEvent draws over entire widget)
    layout->addStretch(1);

    setMinimumSize(700, 500);
}

void PaperSwarmPlot::addEntry(const SwarmPlotEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pointSelected(entry.id, entry.value);
    update();
}

QList<SwarmPlotEntry> PaperSwarmPlot::entries() const { return entries_; }

int PaperSwarmPlot::outlierCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.outlier) ++c;
    return c;
}

qreal PaperSwarmPlot::maxValue() const {
    qreal mx = 0;
    for (const auto& e : entries_)
        if (e.value > mx) mx = e.value;
    return mx;
}

QMap<QString, int> PaperSwarmPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperSwarmPlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"Category-A", "Category-B", "Category-C", "Category-D", "Category-E"};
    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")
    };

    int cIdx = categoryCombo_->currentIndex();
    int catIndex = (cIdx == 0) ? QRandomGenerator::global()->bounded(categories.size()) : (cIdx - 1);
    QString cat = categories[catIndex];

    SwarmPlotEntry e;
    e.id = entries_.size() + 1;
    e.label = text;
    e.category = cat;
    e.group = cat;
    e.x = QRandomGenerator::global()->bounded(10000) / 10000.0;
    e.y = QRandomGenerator::global()->bounded(10000) / 10000.0;
    e.value = QRandomGenerator::global()->bounded(10001) / 100.0;
    e.outlier = e.value > 90;
    e.color = palette[catIndex % palette.size()];

    entries_.append(e);
    saveSettings();
    updateInfo();
    emit pointSelected(e.id, e.value);
    update();
    inputField_->clear();
}

void PaperSwarmPlot::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperSwarmPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add points to see swarm plot");
        return;
    }

    int w = width();
    int h = height();

    // Horizontal three-column layout
    drawSwarmView(p, QRect(20, 10, w * 3 / 5 - 30, h - 20));
    drawCategoryLegend(p, QRect(w * 3 / 5, 10, w / 5 - 10, h - 20));
    drawStats(p, QRect(w * 4 / 5, 10, w / 5 - 20, h - 20));
}

void PaperSwarmPlot::drawSwarmView(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Swarm Plot");

    int margin = 30;
    int topMargin = 40;
    int plotX = rect.x() + margin;
    int plotY = rect.y() + topMargin;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - topMargin - margin;

    if (plotW <= 0 || plotH <= 0) return;

    // Axes
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(plotX, plotY, plotX, plotY + plotH);
    p.drawLine(plotX, plotY + plotH, plotX + plotW, plotY + plotH);

    // Y-axis labels (0-100)
    p.setPen(QColor(148, 163, 184));
    p.setFont(QFont("Arial", 8));
    for (int tick = 0; tick <= 100; tick += 20) {
        int ty = plotY + plotH - static_cast<int>((tick / 100.0) * plotH);
        p.drawText(plotX - 28, ty - 6, 24, 14, Qt::AlignRight | Qt::AlignVCenter, QString::number(tick));
        p.setPen(QPen(QColor(241, 245, 249), 1, Qt::DotLine));
        p.drawLine(plotX + 1, ty, plotX + plotW, ty);
        p.setPen(QColor(148, 163, 184));
    }

    // Draw dots with jitter along x-axis, y = value
    int dotSize = 6;
    for (const auto& e : entries_) {
        // Swarm: spread horizontally with jitter within a band, y mapped from value
        int cx = plotX + static_cast<int>((e.x / 1.0) * plotW);
        int cy = plotY + plotH - static_cast<int>((e.value / 100.0) * plotH);

        // Clamp to plot area
        cx = qBound(plotX, cx, plotX + plotW);
        cy = qBound(plotY, cy, plotY + plotH);

        if (e.outlier) {
            // Outlier: draw with red border
            p.setPen(QPen(QColor("#dc2626"), 2));
            p.setBrush(e.color);
            p.drawEllipse(cx - dotSize / 2, cy - dotSize / 2, dotSize, dotSize);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(e.color);
            p.drawEllipse(cx - dotSize / 2, cy - dotSize / 2, dotSize, dotSize);
        }
    }
}

void PaperSwarmPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Legend");

    static const QStringList categories = {"Category-A", "Category-B", "Category-C", "Category-D", "Category-E"};
    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")
    };

    auto counts = categoryCounts();
    int itemH = qMin(28, (rect.height() - 80) / static_cast<int>(categories.size() + 1));

    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 40 + i * (itemH + 6);

        // Colored square
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 5, y + 2, 14, 14, 2, 2);

        // Category name
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y, rect.width() / 2 - 10, 18, Qt::AlignVCenter, categories[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " pts");
    }

    // Outlier indicator
    int y = rect.y() + 40 + categories.size() * (itemH + 6);
    p.setPen(QPen(QColor("#dc2626"), 2));
    p.setBrush(QColor(239, 239, 239));
    p.drawEllipse(rect.x() + 5, y + 2, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y, rect.width() / 2 - 10, 18, Qt::AlignVCenter, "Outliers");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(outlierCount()));
}

void PaperSwarmPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Points", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Outliers",     QString::number(outlierCount()),  QColor("#dc2626")},
        {"Max Value",    QString::number(maxValue(), 'f', 1), QColor("#d97706")}
    };

    int boxH = qMin(56, (rect.height() - 40) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, boxH / 2, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + boxH / 2, rect.width() - 20, boxH / 2 - 4, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperSwarmPlot::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Points: 0 | Outliers: 0 | Max: 0.0");
        return;
    }
    infoLabel_->setText(QString("Points: %1 | Outliers: %2 | Max: %3")
        .arg(entries_.size())
        .arg(outlierCount())
        .arg(maxValue(), 0, 'f', 1));
}

void PaperSwarmPlot::loadSettings() {
    settings_.beginGroup("SwarmPlot");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SwarmPlotEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group    = settings_.value("group").toString();
        e.x        = settings_.value("x").toDouble();
        e.y        = settings_.value("y").toDouble();
        e.value    = settings_.value("value").toDouble();
        e.outlier  = settings_.value("outlier").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperSwarmPlot::saveSettings() {
    settings_.beginGroup("SwarmPlot");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group",    entries_[i].group);
        settings_.setValue("x",        entries_[i].x);
        settings_.setValue("y",        entries_[i].y);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("outlier",  entries_[i].outlier);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
