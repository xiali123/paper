#include "visualization/PaperAreaChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperAreaChart::PaperAreaChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AreaChart")
{
    setupUI();
    loadSettings();
}

void PaperAreaChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperAreaChart::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Trend", "Volume", "Distribution"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAreaChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate area chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperAreaChart::addEntry(const AreaEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chartGenerated(entry.id, entry.value);
    update();
}

QList<AreaEntry> PaperAreaChart::entries() const { return entries_; }

int PaperAreaChart::positiveCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.positive) c++;
    return c;
}

qreal PaperAreaChart::maxValue() const {
    if (entries_.isEmpty()) return 0;
    qreal mx = entries_[0].value;
    for (const auto& e : entries_) mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperAreaChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperAreaChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"trend", "volume", "distribution"};
    QStringList monthLabels = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    entries_.clear();
    int count = 6 + QRandomGenerator::global()->bounded(5);
    qreal prevBaseline = 30;

    for (int i = 0; i < count; ++i) {
        AreaEntry e;
        e.id = entries_.size() + 1;
        e.label = monthLabels[i % 12];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.value = 20 + QRandomGenerator::global()->bounded(80);
        e.baseline = (i == 0) ? 30 : prevBaseline;
        e.delta = e.value - e.baseline;
        e.order = i;
        e.positive = e.delta >= 0;

        QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
        int cIdx = categories.indexOf(e.category);
        e.color = catColors[cIdx];
        prevBaseline = e.value;
        entries_.append(e);

        saveSettings();
        updateInfo();
        emit chartGenerated(e.id, e.value);
    }
    inputField_->clear();
    update();
}

void PaperAreaChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate area chart");
    update();
}

void PaperAreaChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate area chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Area Chart");

    int w = width(), h = height();
    drawAreaView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAreaChart::drawAreaView(QPainter& p, const QRect& rect) {
    if (entries_.size() < 2) return;

    qreal maxVal = maxValue();
    if (maxVal <= 0) maxVal = 100;
    int n = entries_.size();
    int chartTop = rect.y() + 20;
    int chartBottom = rect.y() + rect.height() - 30;
    int chartLeft = rect.x() + 10;
    int chartRight = rect.x() + rect.width() - 10;
    int chartH = chartBottom - chartTop;

    // Draw filled area
    QPainterPath areaPath;
    qreal xStep = static_cast<qreal>(chartRight - chartLeft) / (n - 1);

    QPointF firstPoint(chartLeft, chartBottom - (entries_[0].value / maxVal) * chartH);
    areaPath.moveTo(firstPoint);
    for (int i = 1; i < n; ++i) {
        qreal x = chartLeft + i * xStep;
        qreal y = chartBottom - (entries_[i].value / maxVal) * chartH;
        areaPath.lineTo(x, y);
    }
    areaPath.lineTo(chartRight, chartBottom);
    areaPath.lineTo(chartLeft, chartBottom);
    areaPath.closeSubpath();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(59, 130, 246, 80));
    p.drawPath(areaPath);

    // Draw value line on top
    QPainterPath linePath;
    linePath.moveTo(firstPoint);
    for (int i = 1; i < n; ++i) {
        qreal x = chartLeft + i * xStep;
        qreal y = chartBottom - (entries_[i].value / maxVal) * chartH;
        linePath.lineTo(x, y);
    }
    p.setPen(QPen(QColor(59, 130, 246), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(linePath);

    // Draw baseline line
    QPainterPath baselinePath;
    qreal baseY = chartBottom - (entries_[0].baseline / maxVal) * chartH;
    baselinePath.moveTo(chartLeft, baseY);
    for (int i = 1; i < n; ++i) {
        qreal x = chartLeft + i * xStep;
        qreal by = chartBottom - (entries_[i].baseline / maxVal) * chartH;
        baselinePath.lineTo(x, by);
    }
    p.setPen(QPen(QColor(203, 213, 225), 1, Qt::DashLine));
    p.drawPath(baselinePath);

    // Draw data points and x-axis labels
    for (int i = 0; i < n; ++i) {
        qreal x = chartLeft + i * xStep;
        qreal y = chartBottom - (entries_[i].value / maxVal) * chartH;

        p.setPen(Qt::NoPen);
        p.setBrush(entries_[i].positive ? QColor(16,185,129) : QColor(239,68,68));
        p.drawEllipse(QPointF(x, y), 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(static_cast<int>(x - 15), chartBottom + 5, 30, 14,
                   Qt::AlignCenter, entries_[i].label);
    }

    // Y-axis labels
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i <= 4; ++i) {
        qreal yVal = maxVal * i / 4;
        int yPos = chartBottom - static_cast<int>((yVal / maxVal) * chartH);
        p.drawText(chartLeft - 5, yPos - 6, 30, 12, Qt::AlignRight,
                   QString::number(static_cast<int>(yVal)));
    }
}

void PaperAreaChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"trend", "volume", "distribution"};
    QString labels[] = {"Trend", "Volume", "Distribution"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperAreaChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Points", QString::number(entries_.size()), QColor(59,130,246)},
        {"Positive", QString::number(positiveCount()), QColor(16,185,129)},
        {"Max", QString::number(maxValue(), 'f', 0), QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperAreaChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate area chart"); return; }
    infoLabel_->setText(QString("%1 points | %2 positive | %3 max")
        .arg(entries_.size()).arg(positiveCount()).arg(maxValue(), 0, 'f', 0));
}

void PaperAreaChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AreaEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.value = settings_.value("value").toDouble();
        e.baseline = settings_.value("baseline").toDouble();
        e.delta = settings_.value("delta").toDouble();
        e.order = settings_.value("order").toInt();
        e.positive = settings_.value("positive").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAreaChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("baseline", entries_[i].baseline);
        settings_.setValue("delta", entries_[i].delta);
        settings_.setValue("order", entries_[i].order);
        settings_.setValue("positive", entries_[i].positive);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
