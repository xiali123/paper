#include "reading/PaperReadingDepth.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QPainterPath>
#include <cmath>

static const QColor kPalette[] = {
    QColor("#3b82f6"),
    QColor("#16a34a"),
    QColor("#d97706"),
    QColor("#dc2626"),
    QColor("#7c3aed"),
};
static constexpr int kPaletteSize = sizeof(kPalette) / sizeof(kPalette[0]);

static QColor nextColor(int index)
{
    return kPalette[index % kPaletteSize];
}

static QString levelForMastery(qreal mastery)
{
    if (mastery >= 0.8) return QStringLiteral("Expert");
    if (mastery >= 0.6) return QStringLiteral("Advanced");
    if (mastery >= 0.4) return QStringLiteral("Intermediate");
    if (mastery >= 0.2) return QStringLiteral("Beginner");
    return QStringLiteral("Novice");
}

PaperReadingDepth::PaperReadingDepth(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingDepth")
    , measureBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingDepth::setupUI()
{
    auto* topLayout = new QHBoxLayout(this);
    topLayout->setContentsMargins(8, 8, 8, 8);
    topLayout->setSpacing(6);

    measureBtn_ = new QPushButton("Measure", this);
    measureBtn_->setFixedWidth(90);
    connect(measureBtn_, &QPushButton::clicked, this, &PaperReadingDepth::onMeasure);

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setFixedWidth(70);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingDepth::onClear);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({
        "General",
        "Machine Learning",
        "NLP",
        "Computer Vision",
        "Systems",
        "Theory",
        "Security",
        "HCI",
    });
    categoryCombo_->setFixedWidth(160);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter paper name to measure reading depth...");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperReadingDepth::onMeasure);

    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setFixedWidth(220);

    topLayout->addWidget(measureBtn_);
    topLayout->addWidget(clearBtn_);
    topLayout->addWidget(categoryCombo_);
    topLayout->addWidget(inputField_, 1);
    topLayout->addWidget(infoLabel_);

    setMinimumHeight(400);
    updateInfo();
}

void PaperReadingDepth::addEntry(const DepthEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<DepthEntry> PaperReadingDepth::entries() const
{
    return entries_;
}

int PaperReadingDepth::expertCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.expert)
            ++count;
    }
    return count;
}

qreal PaperReadingDepth::avgDepth() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.depth;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingDepth::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingDepth::onMeasure()
{
    QString paper = inputField_->text().trimmed();
    if (paper.isEmpty())
        return;

    QString category = categoryCombo_->currentText();

    int id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);

    qreal depth    = QRandomGenerator::global()->bounded(20, 100) / 100.0;
    qreal coverage = QRandomGenerator::global()->bounded(20, 100) / 100.0;
    qreal mastery  = QRandomGenerator::global()->bounded(20, 100) / 100.0;
    bool expert    = mastery > 0.8;

    DepthEntry entry;
    entry.id       = id;
    entry.paper    = paper;
    entry.category = category;
    entry.level    = levelForMastery(mastery);
    entry.depth    = depth;
    entry.coverage = coverage;
    entry.mastery  = mastery;
    entry.expert   = expert;
    entry.color    = nextColor(entries_.size());

    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();

    inputField_->clear();
    emit depthMeasured(id, depth);
}

void PaperReadingDepth::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingDepth::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int toolbarH = 50;

    QRect depthRect(10, toolbarH, w * 55 / 100 - 20, h - toolbarH - 10);
    QRect catRect(w * 55 / 100, toolbarH, w * 45 / 100, (h - toolbarH - 10) / 2);
    QRect statsRect(w * 55 / 100, toolbarH + (h - toolbarH - 10) / 2, w * 45 / 100, (h - toolbarH - 10) / 2);

    drawDepthView(p, depthRect);
    drawCategoryChart(p, catRect);
    drawStats(p, statsRect);
}

void PaperReadingDepth::drawDepthView(QPainter& p, const QRect& rect)
{
    p.save();

    // Background panel
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 12, 12);

    // Border
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 12, 12);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(16, 12, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Reading Depth");

    if (entries_.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        QFont hintFont = font();
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter, "No entries yet.\nEnter a paper name and click Measure.");
        p.restore();
        return;
    }

    int marginLeft   = 50;
    int marginRight   = 20;
    int marginTop     = 45;
    int marginBottom  = 35;
    int chartW = rect.width() - marginLeft - marginRight;
    int chartH = rect.height() - marginTop - marginBottom;

    // Grid lines and Y-axis labels (0.0 to 1.0)
    QFont gridFont = font();
    gridFont.setPointSize(8);
    p.setFont(gridFont);
    p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));

    for (int i = 0; i <= 5; ++i) {
        qreal y = rect.y() + marginTop + chartH - (i * chartH / 5.0);
        p.drawLine(rect.x() + marginLeft, static_cast<int>(y),
                   rect.x() + marginLeft + chartW, static_cast<int>(y));
        p.setPen(QColor(148, 163, 184));
        p.drawText(QRect(rect.x() + 4, static_cast<int>(y) - 8, 42, 16),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(i * 0.2, 'f', 1));
        p.setPen(QPen(QColor(226, 232, 240), 1, Qt::DashLine));
    }

    int barGap = 4;
    int maxBars = qMin(entries_.size(), chartW / (barGap + 14));
    int barW = maxBars > 0 ? (chartW - (maxBars - 1) * barGap) / maxBars : 20;
    barW = qMin(barW, 50);

    int totalBarsW = maxBars * barW + (maxBars - 1) * barGap;
    int startX = rect.x() + marginLeft + (chartW - totalBarsW) / 2;

    for (int i = 0; i < maxBars; ++i) {
        const auto& entry = entries_[entries_.size() - maxBars + i];
        int barH = static_cast<int>(entry.depth * chartH);
        int x = startX + i * (barW + barGap);
        int y = rect.y() + marginTop + chartH - barH;

        // Bar gradient
        QLinearGradient grad(x, y, x, y + barH);
        grad.setColorAt(0.0, entry.color.lighter(120));
        grad.setColorAt(1.0, entry.color);
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawRoundedRect(x, y, barW, barH, 3, 3);

        // Expert marker
        if (entry.expert) {
            p.setBrush(QColor("#facc15"));
            p.drawEllipse(x + barW / 2 - 4, y - 14, 8, 8);
        }

        // X-axis label (truncated paper name)
        if (maxBars <= 20 || i % 2 == 0) {
            p.setPen(QColor(100, 116, 139));
            p.setFont(gridFont);
            QString label = entry.paper;
            if (label.length() > 8)
                label = label.left(7) + "...";
            p.drawText(QRect(x - 4, rect.y() + marginTop + chartH + 4, barW + 8, 20),
                       Qt::AlignCenter, label);
        }
    }

    p.restore();
}

void PaperReadingDepth::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.save();

    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 12, 12);
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 12, 12);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(16, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Category Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        QFont hintFont = font();
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter, "No data");
        p.restore();
        return;
    }

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    int cx = rect.x() + rect.width() / 3;
    int cy = rect.y() + rect.height() / 2 + 8;
    int radius = qMin(rect.width() / 3, rect.height() / 2 - 30);

    qreal startAngle = 0.0;
    int idx = 0;

    QFont labelFont = font();
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it, ++idx) {
        qreal sweep = 360.0 * it.value() / total;
        QColor color = nextColor(idx);

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPie(cx - radius, cy - radius, radius * 2, radius * 2,
                  static_cast<int>(startAngle * 16),
                  static_cast<int>(sweep * 16));

        // Legend entry on the right side
        int legendX = rect.x() + rect.width() * 2 / 3 + 8;
        int legendY = rect.y() + 42 + idx * 20;
        if (legendY + 16 < rect.y() + rect.height()) {
            p.setBrush(color);
            p.drawRoundedRect(legendX, legendY, 12, 12, 2, 2);
            p.setPen(QColor(51, 65, 85));
            p.drawText(legendX + 18, legendY + 11,
                       QString("%1 (%2)").arg(it.key()).arg(it.value()));
        }

        startAngle += sweep;
    }

    p.restore();
}

void PaperReadingDepth::drawStats(QPainter& p, const QRect& rect)
{
    p.save();

    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 12, 12);
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(rect, 12, 12);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(16, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statFont = font();
    statFont.setPointSize(10);
    p.setFont(statFont);

    int y = rect.y() + 36;
    int lineH = 22;
    int leftX = rect.x() + 20;

    auto drawRow = [&](const QString& label, const QString& value, const QColor& valueColor) {
        p.setPen(QColor(100, 116, 139));
        p.drawText(leftX, y, label);
        p.setPen(valueColor);
        int labelW = p.fontMetrics().horizontalAdvance(label) + 8;
        p.drawText(leftX + labelW, y, value);
        y += lineH;
    };

    drawRow("Total papers: ", QString::number(entries_.size()), QColor(51, 65, 85));
    drawRow("Avg depth: ", QString::number(avgDepth(), 'f', 2), QColor("#3b82f6"));
    drawRow("Expert count: ", QString::number(expertCount()), QColor("#16a34a"));

    // Mastery distribution bars
    y += 8;
    p.setPen(QColor(30, 41, 59));
    p.setFont(titleFont);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(leftX, y, "Mastery Distribution");
    y += 20;

    int nov = 0, beg = 0, inter = 0, adv = 0, exp = 0;
    for (const auto& e : entries_) {
        if (e.mastery >= 0.8)       ++exp;
        else if (e.mastery >= 0.6)  ++adv;
        else if (e.mastery >= 0.4)  ++inter;
        else if (e.mastery >= 0.2)  ++beg;
        else                        ++nov;
    }

    struct Tier { QString name; int count; QColor color; };
    Tier tiers[] = {
        {"Novice",       nov,   QColor("#dc2626")},
        {"Beginner",     beg,   QColor("#d97706")},
        {"Intermediate", inter, QColor("#3b82f6")},
        {"Advanced",     adv,   QColor("#16a34a")},
        {"Expert",       exp,   QColor("#7c3aed")},
    };

    int barMaxW = rect.width() - 160;
    int maxCount = entries_.size() > 0 ? entries_.size() : 1;

    p.setFont(statFont);
    for (const auto& tier : tiers) {
        p.setPen(QColor(100, 116, 139));
        p.drawText(leftX, y, tier.name);
        int nameW = p.fontMetrics().horizontalAdvance(tier.name) + 8;

        int bw = static_cast<int>(tier.count * barMaxW / maxCount);
        if (bw > 0) {
            p.setPen(Qt::NoPen);
            p.setBrush(tier.color);
            p.drawRoundedRect(leftX + nameW, y - 12, bw, 14, 3, 3);
        }

        p.setPen(QColor(51, 65, 85));
        p.drawText(leftX + nameW + bw + 6, y, QString::number(tier.count));
        y += lineH;
    }

    p.restore();
}

void PaperReadingDepth::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText("No entries yet.");
        return;
    }
    infoLabel_->setText(
        QString("Papers: %1 | Avg depth: %2 | Experts: %3")
            .arg(entries_.size())
            .arg(avgDepth(), 0, 'f', 2)
            .arg(expertCount()));
}

void PaperReadingDepth::loadSettings()
{
    int size = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DepthEntry e;
        e.id       = settings_.value("id").toInt();
        e.paper    = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.level    = settings_.value("level").toString();
        e.depth    = settings_.value("depth").toDouble();
        e.coverage = settings_.value("coverage").toDouble();
        e.mastery  = settings_.value("mastery").toDouble();
        e.expert   = settings_.value("expert").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
    update();
}

void PaperReadingDepth::saveSettings()
{
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id",       e.id);
        settings_.setValue("paper",    e.paper);
        settings_.setValue("category", e.category);
        settings_.setValue("level",    e.level);
        settings_.setValue("depth",    e.depth);
        settings_.setValue("coverage", e.coverage);
        settings_.setValue("mastery",  e.mastery);
        settings_.setValue("expert",   e.expert);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
