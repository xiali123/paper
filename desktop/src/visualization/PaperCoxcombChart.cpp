#include "visualization/PaperCoxcombChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QtMath>
#include <numeric>

namespace {
static const QColor kPalette[] = {
    QColor(0x3b, 0x82, 0xf6),
    QColor(0x16, 0xa3, 0x4a),
    QColor(0xd9, 0x77, 0x06),
    QColor(0xdc, 0x26, 0x26),
    QColor(0x7c, 0x3a, 0xed),
};
static constexpr int kPaletteSize = 5;
}

PaperCoxcombChart::PaperCoxcombChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CoxcombChart")
{
    setupUI();
    loadSettings();
}

void PaperCoxcombChart::addEntry(const CoxcombEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit wedgeSelected(entry.id, entry.value);
    update();
}

QList<CoxcombEntry> PaperCoxcombChart::entries() const { return entries_; }

int PaperCoxcombChart::peakCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.peak) c++;
    return c;
}

qreal PaperCoxcombChart::maxValue() const {
    if (entries_.isEmpty()) return 0;
    qreal mx = entries_[0].value;
    for (const auto& e : entries_) mx = qMax(mx, e.value);
    return mx;
}

QMap<QString, int> PaperCoxcombChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCoxcombChart::onRender() {
    entries_.clear();

    QStringList categories = {"impact", "novelty", "methodology", "citation", "relevance"};
    QStringList wedges = {"Q1", "Q2", "Q3", "Q4", "annual", "biannual", "monthly", "weekly", "daily", "quarterly"};
    QStringList labels = {"Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta", "Iota", "Kappa"};

    int count = 5 + QRandomGenerator::global()->bounded(6);
    qreal angleStep = 360.0 / count;
    qreal nextAngle = 0.0;

    QString selectedCategory = categoryCombo_->currentText().toLower();
    if (selectedCategory == "all") selectedCategory.clear();

    QVector<qreal> rawValues;
    rawValues.reserve(count);
    for (int i = 0; i < count; ++i) {
        rawValues << 10.0 + QRandomGenerator::global()->bounded(90);
    }

    QVector<qreal> sorted = rawValues;
    std::sort(sorted.begin(), sorted.end(), std::greater<qreal>());
    int topN = qMax(1, static_cast<int>(std::ceil(count * 0.2)));
    qreal threshold = sorted[qMin(topN - 1, sorted.size() - 1)];

    for (int i = 0; i < count; ++i) {
        CoxcombEntry e;
        e.id = i + 1;
        e.label = labels[i % labels.size()];
        e.category = selectedCategory.isEmpty()
                         ? categories[i % categories.size()]
                         : selectedCategory;
        e.wedge = wedges[i % wedges.size()];
        e.value = rawValues[i];
        e.angle = nextAngle;
        e.peak = (e.value >= threshold);
        e.color = kPalette[i % kPaletteSize];
        entries_.append(e);
        nextAngle += angleStep;
    }

    saveSettings();
    updateInfo();

    for (const auto& e : entries_) {
        emit wedgeSelected(e.id, e.value);
    }

    inputField_->clear();
    update();
}

void PaperCoxcombChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Click Render to generate coxcomb chart");
    update();
}

void PaperCoxcombChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Click Render to generate coxcomb chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Coxcomb Chart");

    int w = width(), h = height();
    drawCoxcomb(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCoxcombChart::drawCoxcomb(QPainter& p, const QRect& chartRect) {
    int n = entries_.size();
    if (n == 0) return;

    qreal maxVal = maxValue();
    if (maxVal <= 0) maxVal = 100.0;

    int side = qMin(chartRect.width(), chartRect.height()) - 40;
    if (side <= 0) return;
    qreal cx = chartRect.x() + chartRect.width() / 2.0;
    qreal cy = chartRect.y() + chartRect.height() / 2.0;
    qreal maxR = side / 2.0;

    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DotLine));
    p.setBrush(Qt::NoBrush);
    for (int ring = 1; ring <= 4; ++ring) {
        qreal r = maxR * ring / 4.0;
        p.drawEllipse(QPointF(cx, cy), r, r);
    }

    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    p.drawLine(QPointF(cx - maxR, cy), QPointF(cx + maxR, cy));
    p.drawLine(QPointF(cx, cy - maxR), QPointF(cx, cy + maxR));

    qreal angleStep = 360.0 / n;

    for (int i = 0; i < n; ++i) {
        const CoxcombEntry& e = entries_[i];
        qreal barRadius = (e.value / maxVal) * maxR;
        qreal qtStart = (90.0 - e.angle - angleStep) * 16.0;
        qreal qtSpan = angleStep * 16.0;

        QPainterPath wedge;
        wedge.moveTo(cx, cy);
        wedge.arcTo(QRectF(cx - barRadius, cy - barRadius,
                           barRadius * 2, barRadius * 2),
                    qtStart / 16.0, -angleStep);
        wedge.closeSubpath();

        QColor fill = e.color;
        fill.setAlpha(e.peak ? 180 : 100);
        p.setPen(QPen(e.color, e.peak ? 2 : 1));
        p.setBrush(fill);
        p.drawPath(wedge);

        if (e.peak) {
            p.setPen(QPen(e.color.darker(120), 3));
            p.setBrush(Qt::NoBrush);
            p.drawArc(QRectF(cx - barRadius, cy - barRadius,
                             barRadius * 2, barRadius * 2),
                      static_cast<int>(qtStart), static_cast<int>(-qtSpan));
        }

        qreal labelAngle = qDegreesToRadians(e.angle + angleStep / 2.0);
        qreal labelR = maxR + 14;
        qreal lx = cx + labelR * qCos(labelAngle);
        qreal ly = cy - labelR * qSin(labelAngle);

        p.setPen(e.peak ? QColor(15, 23, 42) : QColor(100, 116, 139));
        p.setFont(e.peak ? QFont("Arial", 8, QFont::Bold) : QFont("Arial", 7));
        p.drawText(QRectF(lx - 30, ly - 8, 60, 16), Qt::AlignCenter, e.label);

        qreal valR = barRadius * 0.55;
        qreal vx = cx + valR * qCos(labelAngle);
        qreal vy = cy - valR * qSin(labelAngle);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRectF(vx - 18, vy - 7, 36, 14), Qt::AlignCenter,
                   QString::number(e.value, 'f', 0));
    }

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(15, 23, 42));
    p.drawEllipse(QPointF(cx, cy), 4, 4);
}

void PaperCoxcombChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = counts.keys();

    if (cats.isEmpty()) {
        cats = QStringList{"impact", "novelty", "methodology", "citation", "relevance"};
    }

    QString displayLabels[] = {"Impact", "Novelty", "Methodology", "Citation", "Relevance"};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int catCount = qMin(cats.size(), 5);
    int barH = qMin(22, (rect.height() - 30) / qMax(catCount, 1));

    for (int i = 0; i < catCount; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        QString label = (i < 5) ? displayLabels[i] : cats[i];
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, label);

        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i % kPaletteSize]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCoxcombChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",   QString::number(entries_.size()),    QColor(0x3b, 0x82, 0xf6)},
        {"Peak",      QString::number(peakCount()),        QColor(0x16, 0xa3, 0x4a)},
        {"Max Value", QString::number(maxValue(), 'f', 1), QColor(0xd9, 0x77, 0x06)},
        {"Avg Value", QString::number(
             entries_.isEmpty() ? 0.0
                 : std::accumulate(entries_.begin(), entries_.end(), 0.0,
                       [](double s, const CoxcombEntry& e) { return s + e.value; })
                 / entries_.size(), 'f', 1),
         QColor(0x7c, 0x3a, 0xed)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0xdc, 0x26, 0x26)}
    };

    int boxH = qMin(36, (rect.height() - 10) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 4);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 13, QFont::Bold));
        p.drawText(rect.x() + 10, y + 3, rect.width() - 20, 20,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 10, y + 20, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperCoxcombChart::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Click Render to generate coxcomb chart");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 peak | max %3")
        .arg(entries_.size())
        .arg(peakCount())
        .arg(maxValue(), 0, 'f', 1));
}

void PaperCoxcombChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CoxcombEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.wedge    = settings_.value("wedge").toString();
        e.value    = settings_.value("value").toDouble();
        e.angle    = settings_.value("angle").toDouble();
        e.peak     = settings_.value("peak").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCoxcombChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("wedge",    entries_[i].wedge);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("angle",    entries_[i].angle);
        settings_.setValue("peak",     entries_[i].peak);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}

void PaperCoxcombChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperCoxcombChart::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Impact", "Novelty", "Methodology", "Citation"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter filter or leave empty to randomize...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCoxcombChart::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Click Render to generate coxcomb chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}
