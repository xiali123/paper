#include "visualization/PaperAreaChart2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperAreaChart2::PaperAreaChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AreaChart2")
{
    setupUI();
    loadSettings();
}

void PaperAreaChart2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperAreaChart2::onRender);
    toolbar->addWidget(renderBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Trend", "Volume", "Stacked", "Baseline", "Series"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAreaChart2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render area chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperAreaChart2::addEntry(const Area2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit areaSelected(entry.id, entry.value);
    update();
}

QList<Area2Entry> PaperAreaChart2::entries() const { return entries_; }

int PaperAreaChart2::stackedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.stacked) c++;
    return c;
}

qreal PaperAreaChart2::maxValue() const {
    if (entries_.isEmpty()) return 0;
    qreal mx = entries_[0].value;
    for (const auto& e : entries_) mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperAreaChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperAreaChart2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    QStringList categories = {"trend", "volume", "stacked", "baseline", "series"};
    QStringList monthLabels = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                               "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(6);
    qreal prevBaseline = 20;

    for (int i = 0; i < count; ++i) {
        Area2Entry e;
        e.id = entries_.size() + 1;
        e.label = monthLabels[i % 12];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.series = QString("S%1").arg(QRandomGenerator::global()->bounded(3) + 1);
        e.value = 20 + QRandomGenerator::global()->bounded(80);
        e.baseline = (i == 0) ? prevBaseline : prevBaseline + QRandomGenerator::global()->bounded(20) - 10;
        e.stacked = QRandomGenerator::global()->bounded(2) == 0;
        e.color = palette[i % 5];
        prevBaseline = e.value;
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    for (const auto& e : entries_) emit areaSelected(e.id, e.value);
    inputField_->clear();
    update();
}

void PaperAreaChart2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render area chart");
    update();
}

void PaperAreaChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render area chart");
        return;
    }

    int w = width(), h = height();
    int topMargin = 50;
    int colW = (w - 60) / 3;

    drawAreaChart(p, QRect(20, topMargin, colW, h - topMargin - 20));
    drawCategoryLegend(p, QRect(20 + colW + 10, topMargin, colW, h - topMargin - 20));
    drawStats(p, QRect(20 + 2 * (colW + 10), topMargin, colW, h - topMargin - 20));
}

void PaperAreaChart2::drawAreaChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Area Chart");

    if (entries_.size() < 2) return;

    qreal maxVal = maxValue();
    if (maxVal <= 0) maxVal = 100;
    int n = entries_.size();
    int chartTop = rect.y() + 30;
    int chartBottom = rect.y() + rect.height() - 30;
    int chartLeft = rect.x() + 10;
    int chartRight = rect.x() + rect.width() - 10;
    int chartH = chartBottom - chartTop;
    qreal xStep = static_cast<qreal>(chartRight - chartLeft) / (n - 1);

    for (int s = 0; s < 2; ++s) {
        bool drawBaseline = (s == 1);
        QPainterPath areaPath;
        qreal firstY = chartBottom - ((drawBaseline ? entries_[0].baseline : entries_[0].value) / maxVal) * chartH;
        QPointF firstPoint(chartLeft, firstY);
        areaPath.moveTo(firstPoint);
        for (int i = 1; i < n; ++i) {
            qreal x = chartLeft + i * xStep;
            qreal y = chartBottom - ((drawBaseline ? entries_[i].baseline : entries_[i].value) / maxVal) * chartH;
            areaPath.lineTo(x, y);
        }
        areaPath.lineTo(chartRight, chartBottom);
        areaPath.lineTo(chartLeft, chartBottom);
        areaPath.closeSubpath();

        if (drawBaseline) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(203, 213, 225, 60));
            p.drawPath(areaPath);
        } else {
            p.setPen(Qt::NoPen);
            QColor fill = entries_[0].color;
            fill.setAlpha(80);
            p.setBrush(fill);
            p.drawPath(areaPath);

            QPainterPath linePath;
            linePath.moveTo(firstPoint);
            for (int i = 1; i < n; ++i) {
                qreal x = chartLeft + i * xStep;
                qreal y = chartBottom - (entries_[i].value / maxVal) * chartH;
                linePath.lineTo(x, y);
            }
            p.setPen(QPen(entries_[0].color, 2));
            p.setBrush(Qt::NoBrush);
            p.drawPath(linePath);
        }
    }

    for (int i = 0; i < n; ++i) {
        qreal x = chartLeft + i * xStep;
        qreal y = chartBottom - (entries_[i].value / maxVal) * chartH;

        p.setPen(Qt::NoPen);
        p.setBrush(entries_[i].stacked ? QColor(16, 185, 129) : QColor(239, 68, 68));
        p.drawEllipse(QPointF(x, y), 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(static_cast<int>(x - 15), chartBottom + 5, 30, 14,
                   Qt::AlignCenter, entries_[i].label);
    }

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i <= 4; ++i) {
        qreal yVal = maxVal * i / 4;
        int yPos = chartBottom - static_cast<int>((yVal / maxVal) * chartH);
        p.drawText(chartLeft - 5, yPos - 6, 30, 12, Qt::AlignRight,
                   QString::number(static_cast<int>(yVal)));
    }
}

void PaperAreaChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"trend", "volume", "stacked", "baseline", "series"};
    QString labels[] = {"Trend", "Volume", "Stacked", "Baseline", "Series"};
    QColor colors[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
                       QColor("#dc2626"), QColor("#7c3aed")};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 40) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 30 + i * (barH + 4);
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

void PaperAreaChart2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Points", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Stacked", QString::number(stackedCount()), QColor("#16a34a")},
        {"Max", QString::number(maxValue(), 'f', 0), QColor("#d97706")},
        {"Categories", QString::number(categoryCounts().size()), QColor("#dc2626")}
    };

    int boxH = qMin(50, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperAreaChart2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render area chart"); return; }
    infoLabel_->setText(QString("%1 points | %2 stacked | %3 max")
        .arg(entries_.size()).arg(stackedCount()).arg(maxValue(), 0, 'f', 0));
}

void PaperAreaChart2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        Area2Entry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.series = settings_.value("series").toString();
        e.value = settings_.value("value").toDouble();
        e.baseline = settings_.value("baseline").toDouble();
        e.stacked = settings_.value("stacked").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAreaChart2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("series", entries_[i].series);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("baseline", entries_[i].baseline);
        settings_.setValue("stacked", entries_[i].stacked);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
