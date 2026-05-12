#include "visualization/PaperDonutChart2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperDonutChart2::PaperDonutChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DonutChart2")
{
    setupUI();
    loadSettings();
}

void PaperDonutChart2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "ML", "NLP", "CV", "Robotics", "Theory", "Data"});

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperDonutChart2::onRender);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDonutChart2::onClear);

    infoLabel_ = new QLabel("Render donut chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");

    mainLayout->addWidget(categoryCombo_);
    mainLayout->addWidget(inputField_);
    mainLayout->addWidget(renderBtn_);
    mainLayout->addWidget(clearBtn_);
    mainLayout->addWidget(infoLabel_);
    mainLayout->addStretch();

    setMinimumSize(600, 500);
}

void PaperDonutChart2::addEntry(const DonutEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<DonutEntry> PaperDonutChart2::entries() const {
    return entries_;
}

int PaperDonutChart2::largestCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.largest) ++count;
    }
    return count;
}

qreal PaperDonutChart2::maxValue() const {
    qreal maxVal = 0;
    for (const auto& e : entries_) {
        if (e.value > maxVal) maxVal = e.value;
    }
    return maxVal;
}

QMap<QString, int> PaperDonutChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperDonutChart2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    DonutEntry entry;
    entry.id = entries_.size() + 1;
    entry.label = text;
    entry.category = categoryCombo_->currentText();
    entry.segment = entry.category;
    entry.value = 1 + QRandomGenerator::global()->bounded(100);

    int totalSoFar = 0;
    for (const auto& e : entries_) totalSoFar += static_cast<int>(e.value);
    int newTotal = totalSoFar + static_cast<int>(entry.value);
    entry.percentage = entry.value / static_cast<qreal>(newTotal);

    entry.rank = entries_.size() + 1;

    qreal maxVal = 0;
    for (const auto& e : entries_) {
        if (e.value > maxVal) maxVal = e.value;
    }
    entry.largest = (entry.value >= maxVal);

    entry.color = palette[entries_.size() % 5];

    entries_.append(entry);

    // Recompute percentages relative to total
    qreal total = 0;
    for (const auto& e : entries_) total += e.value;
    for (auto& e : entries_) e.percentage = e.value / qMax(total, 1.0);

    // Determine largest flag
    qreal mv = 0;
    for (const auto& e : entries_) { if (e.value > mv) mv = e.value; }
    for (auto& e : entries_) e.largest = (e.value >= mv);

    saveSettings();
    updateInfo();
    emit segmentSelected(entry.id, entry.value);
    update();
    inputField_->clear();
}

void PaperDonutChart2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperDonutChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render donut chart");
        return;
    }

    int w = width(), h = height();

    // Top area for toolbar
    int topMargin = 50;
    int colW = (w - 60) / 3;

    drawDonutView(p, QRect(20, topMargin, colW, h - topMargin - 20));
    drawCategoryLegend(p, QRect(20 + colW + 10, topMargin, colW, h - topMargin - 20));
    drawStats(p, QRect(20 + 2 * (colW + 10), topMargin, colW, h - topMargin - 20));
}

void PaperDonutChart2::drawDonutView(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Donut Chart");

    qreal total = 0;
    for (const auto& e : entries_) total += e.value;
    if (total == 0) return;

    int pieSize = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 40 + (rect.height() - 40) / 2;
    int outerR = pieSize / 2;
    int innerR = outerR / 2;

    qreal startAngle = 0;
    for (const auto& e : entries_) {
        qreal span = (e.value / total) * 360.0;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));

        // Percentage label on larger segments
        if (e.percentage > 0.08) {
            qreal midAngle = startAngle + span / 2.0;
            qreal rad = qDegreesToRadians(midAngle);
            int labelR = (outerR + innerR) / 2;
            int tx = cx + static_cast<int>(labelR * qCos(rad));
            int ty = cy - static_cast<int>(labelR * qSin(rad));
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(tx - 20, ty - 5, 40, 14, Qt::AlignCenter,
                       QString::number(e.percentage * 100, 'f', 0) + "%");
        }
        startAngle += span;
    }

    // Inner circle (donut hole)
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text: total
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    QString totalText = QString::number(static_cast<int>(total));
    QFontMetrics fm(p.font());
    int tw = fm.horizontalAdvance(totalText);
    p.drawText(cx - tw / 2, cy + fm.ascent() / 2 - 2, totalText);
}

void PaperDonutChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Legend");

    int startY = rect.y() + 30;
    int itemH = 24;
    int maxItems = (rect.height() - 30) / itemH;
    int show = qMin(entries_.size(), maxItems);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = startY + i * itemH;

        // Colored square
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 12, 12, 2, 2);

        // Label text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 18, y + 1, rect.width() - 22, 14, Qt::AlignVCenter, e.label);
    }

    if (entries_.size() > show) {
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), startY + show * itemH,
                   QString("+ %1 more").arg(entries_.size() - show));
    }
}

void PaperDonutChart2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Segments", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Largest Count",  QString::number(largestCount()),  QColor("#16a34a")},
        {"Max Value",      QString::number(maxValue(), 'f', 1), QColor("#d97706")}
    };

    int boxH = qMin(50, (rect.height() - 10) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 6);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 14, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperDonutChart2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render donut chart");
        return;
    }
    infoLabel_->setText(
        QString("Segments: %1 | Largest: %2 | Max: %3")
            .arg(entries_.size())
            .arg(largestCount())
            .arg(maxValue(), 0, 'f', 1));
}

void PaperDonutChart2::loadSettings() {
    settings_.beginGroup("DonutChart2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DonutEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.segment = settings_.value("segment").toString();
        e.value = settings_.value("value").toDouble();
        e.percentage = settings_.value("percentage").toDouble();
        e.rank = settings_.value("rank").toInt();
        e.largest = settings_.value("largest").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperDonutChart2::saveSettings() {
    settings_.beginGroup("DonutChart2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("segment", entries_[i].segment);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("percentage", entries_[i].percentage);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("largest", entries_[i].largest);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
