#include "visualization/PaperCoxcombChart2.hpp"
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

PaperCoxcombChart2::PaperCoxcombChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CoxcombChart2")
{
    setupUI();
    loadSettings();
}

void PaperCoxcombChart2::addEntry(const CoxcombChart2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sectorSelected(entry.id, entry.angle);
    update();
}

QList<CoxcombChart2Entry> PaperCoxcombChart2::entries() const { return entries_; }

int PaperCoxcombChart2::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dominant) c++;
    return c;
}

qreal PaperCoxcombChart2::maxAngle() const {
    if (entries_.isEmpty()) return 0;
    qreal mx = entries_[0].angle;
    for (const auto& e : entries_) mx = qMax(mx, e.angle);
    return mx;
}

QMap<QString, int> PaperCoxcombChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCoxcombChart2::onRender() {
    entries_.clear();

    QStringList sectors   = {"Research", "Teaching", "Service", "Outreach"};
    QStringList metrics   = {"Publications", "Grants", "Students", "Citations"};
    QStringList categories = {"Academic", "Industry", "Government", "Medical", "Non-profit"};

    QString selectedCategory = categoryCombo_->currentText();
    if (selectedCategory == "All") selectedCategory.clear();

    int count = 8;
    qreal angleStep = 360.0 / count;
    qreal nextAngle = 0.0;

    QVector<int> rawFreq;
    rawFreq.reserve(count);
    for (int i = 0; i < count; ++i) {
        rawFreq << 10 + QRandomGenerator::global()->bounded(90);
    }

    QVector<int> sorted = rawFreq;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    int topN = qMax(1, static_cast<int>(std::ceil(count * 0.3)));
    int threshold = sorted[qMin(topN - 1, sorted.size() - 1)];

    for (int i = 0; i < count; ++i) {
        CoxcombChart2Entry e;
        e.id       = i + 1;
        e.sector   = sectors[i % sectors.size()];
        e.category = selectedCategory.isEmpty()
                         ? categories[i % categories.size()]
                         : selectedCategory;
        e.metric   = metrics[i % metrics.size()];
        e.angle    = nextAngle;
        e.frequency = rawFreq[i];
        e.dominant = (e.frequency >= threshold);
        e.color    = kPalette[i % kPaletteSize];
        entries_.append(e);
        nextAngle += angleStep;
    }

    saveSettings();
    updateInfo();

    for (const auto& e : entries_) {
        emit sectorSelected(e.id, e.angle);
    }

    inputField_->clear();
    update();
}

void PaperCoxcombChart2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Click Render to generate coxcomb chart");
    update();
}

void PaperCoxcombChart2::paintEvent(QPaintEvent*) {
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
    p.drawText(20, 30, "Coxcomb Chart 2 — Nightingale Diagram");

    int w = width(), h = height();
    drawCoxcomb(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCoxcombChart2::drawCoxcomb(QPainter& p, const QRect& chartRect) {
    int n = entries_.size();
    if (n == 0) return;

    int maxFreq = 1;
    for (const auto& e : entries_) maxFreq = qMax(maxFreq, e.frequency);

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
        const CoxcombChart2Entry& e = entries_[i];
        qreal barRadius = (static_cast<qreal>(e.frequency) / maxFreq) * maxR;
        qreal qtStart = (90.0 - e.angle - angleStep) * 16.0;
        qreal qtSpan  = angleStep * 16.0;

        QPainterPath wedge;
        wedge.moveTo(cx, cy);
        wedge.arcTo(QRectF(cx - barRadius, cy - barRadius,
                           barRadius * 2, barRadius * 2),
                    qtStart / 16.0, -angleStep);
        wedge.closeSubpath();

        QColor fill = e.color;
        fill.setAlpha(e.dominant ? 190 : 110);
        p.setPen(QPen(e.color, e.dominant ? 2 : 1));
        p.setBrush(fill);
        p.drawPath(wedge);

        if (e.dominant) {
            p.setPen(QPen(e.color.darker(130), 3));
            p.setBrush(Qt::NoBrush);
            p.drawArc(QRectF(cx - barRadius, cy - barRadius,
                             barRadius * 2, barRadius * 2),
                      static_cast<int>(qtStart), static_cast<int>(-qtSpan));
        }

        qreal labelAngle = qDegreesToRadians(e.angle + angleStep / 2.0);
        qreal labelR = maxR + 16;
        qreal lx = cx + labelR * qCos(labelAngle);
        qreal ly = cy - labelR * qSin(labelAngle);

        p.setPen(e.dominant ? QColor(15, 23, 42) : QColor(100, 116, 139));
        p.setFont(e.dominant ? QFont("Arial", 8, QFont::Bold) : QFont("Arial", 7));
        p.drawText(QRectF(lx - 32, ly - 8, 64, 16), Qt::AlignCenter, e.sector);

        qreal valR = barRadius * 0.55;
        qreal vx = cx + valR * qCos(labelAngle);
        qreal vy = cy - valR * qSin(labelAngle);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRectF(vx - 20, vy - 7, 40, 14), Qt::AlignCenter,
                   QString::number(e.frequency));
    }

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(15, 23, 42));
    p.drawEllipse(QPointF(cx, cy), 5, 5);
}

void PaperCoxcombChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = counts.keys();

    if (cats.isEmpty()) {
        cats = QStringList{"Academic", "Industry", "Government", "Medical", "Non-profit"};
    }

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int catCount = qMin(cats.size(), 5);
    int barH = qMin(22, (rect.height() - 30) / qMax(catCount, 1));

    for (int i = 0; i < catCount; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 72, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i % kPaletteSize]);
        p.drawRoundedRect(rect.x() + 76, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 79 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCoxcombChart2::drawStats(QPainter& p, const QRect& rect) {
    qreal avgFreq = entries_.isEmpty() ? 0.0
        : std::accumulate(entries_.begin(), entries_.end(), 0.0,
              [](double s, const CoxcombChart2Entry& e) { return s + e.frequency; })
          / entries_.size();

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",    QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Dominant",   QString::number(dominantCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Max Freq",   QString::number(
             entries_.isEmpty() ? 0
                 : std::max_element(entries_.begin(), entries_.end(),
                       [](const CoxcombChart2Entry& a, const CoxcombChart2Entry& b) {
                           return a.frequency < b.frequency;
                       })->frequency),
         QColor(0xd9, 0x77, 0x06)},
        {"Avg Freq",   QString::number(avgFreq, 'f', 1), QColor(0x7c, 0x3a, 0xed)},
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

void PaperCoxcombChart2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Click Render to generate coxcomb chart");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 dominant | max freq %3")
        .arg(entries_.size())
        .arg(dominantCount())
        .arg(entries_.isEmpty() ? 0
            : std::max_element(entries_.begin(), entries_.end(),
                  [](const CoxcombChart2Entry& a, const CoxcombChart2Entry& b) {
                      return a.frequency < b.frequency;
                  })->frequency));
}

void PaperCoxcombChart2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CoxcombChart2Entry e;
        e.id        = settings_.value("id").toInt();
        e.sector    = settings_.value("sector").toString();
        e.category  = settings_.value("category").toString();
        e.metric    = settings_.value("metric").toString();
        e.angle     = settings_.value("angle").toDouble();
        e.frequency = settings_.value("frequency").toInt();
        e.dominant  = settings_.value("dominant").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCoxcombChart2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",        entries_[i].id);
        settings_.setValue("sector",    entries_[i].sector);
        settings_.setValue("category",  entries_[i].category);
        settings_.setValue("metric",    entries_[i].metric);
        settings_.setValue("angle",     entries_[i].angle);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("dominant",  entries_[i].dominant);
        settings_.setValue("color",     entries_[i].color.name());
    }
    settings_.endArray();
}

void PaperCoxcombChart2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperCoxcombChart2::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Academic", "Industry", "Government", "Medical", "Non-profit"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter filter or leave empty to randomize...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCoxcombChart2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Click Render to generate coxcomb chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}
