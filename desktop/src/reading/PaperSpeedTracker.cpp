#include "reading/PaperSpeedTracker.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <QPaintEvent>
#include <cmath>

namespace {

QColor categoryColor(const QString& category) {
    if (category == "Skimming") return QColor("#3b82f6");
    if (category == "Normal")    return QColor("#16a34a");
    if (category == "Deep")      return QColor("#7c3aed");
    if (category == "Speed")     return QColor("#d97706");
    return QColor("#64748b");
}

QString randomMethod() {
    static const QStringList methods = {
        "Active Reading", "SQ3R", "PQ4R", "Skim-Read", "Scan-Read",
        "Detailed Notes", "Highlight + Review", "Mind Mapping", "Summarization"
    };
    return methods[QRandomGenerator::global()->bounded(methods.size())];
}

} // namespace

PaperSpeedTracker::PaperSpeedTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SpeedTracker")
    , recordBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperSpeedTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Skimming", "Normal", "Deep", "Speed"});
    categoryCombo_->setMinimumWidth(110);
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter paper...");
    toolbar->addWidget(inputField_, 1);

    recordBtn_ = new QPushButton("Record", this);
    recordBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px;"
        " border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(recordBtn_, &QPushButton::clicked, this, &PaperSpeedTracker::onRecord);
    toolbar->addWidget(recordBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 10px; border: 1px solid #fca5a5;"
        " border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSpeedTracker::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px 2px;");
    infoLabel_->setWordWrap(true);
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
    updateInfo();
}

void PaperSpeedTracker::addEntry(const SpeedTrackerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit speedRecorded(entry.id, entry.wpm);
    update();
}

QList<SpeedTrackerEntry> PaperSpeedTracker::entries() const {
    return entries_;
}

int PaperSpeedTracker::aboveAverageCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.aboveAverage) ++count;
    }
    return count;
}

qreal PaperSpeedTracker::avgWpm() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.wpm;
    return sum / entries_.size();
}

QMap<QString, int> PaperSpeedTracker::categoryCounts() const {
    QMap<QString, int> counts;
    counts["Skimming"] = 0;
    counts["Normal"] = 0;
    counts["Deep"] = 0;
    counts["Speed"] = 0;
    for (const auto& e : entries_) {
        if (counts.contains(e.category)) {
            counts[e.category]++;
        }
    }
    return counts;
}

void PaperSpeedTracker::onRecord() {
    QString paper = inputField_->text().trimmed();
    if (paper.isEmpty()) return;

    int catIdx = categoryCombo_->currentIndex();
    QString category;
    if (catIdx == 1) category = "Skimming";
    else if (catIdx == 2) category = "Normal";
    else if (catIdx == 3) category = "Deep";
    else if (catIdx == 4) category = "Speed";
    else category = QStringList{"Skimming", "Normal", "Deep", "Speed"}[
        QRandomGenerator::global()->bounded(4)];

    qreal baseWpm = 0;
    if (category == "Skimming") baseWpm = 400 + QRandomGenerator::global()->bounded(200);
    else if (category == "Normal") baseWpm = 200 + QRandomGenerator::global()->bounded(150);
    else if (category == "Deep") baseWpm = 80 + QRandomGenerator::global()->bounded(120);
    else if (category == "Speed") baseWpm = 500 + QRandomGenerator::global()->bounded(300);

    int pages = 2 + QRandomGenerator::global()->bounded(30);

    SpeedTrackerEntry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.paper = paper;
    entry.category = category;
    entry.method = randomMethod();
    entry.wpm = baseWpm;
    entry.pagesRead = pages;
    entry.color = categoryColor(category);

    qreal currentAvg = avgWpm();
    entry.aboveAverage = (currentAvg <= 0.0) ? true : (entry.wpm >= currentAvg);

    addEntry(entry);
    inputField_->clear();
}

void PaperSpeedTracker::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperSpeedTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "Record reading speed entries to begin tracking");
        return;
    }

    int w = width();
    int h = height();
    int halfW = w / 2;
    int halfH = h / 2;

    int topWidgetH = qMax(160, h * 35 / 100);
    int toolbarOffset = 90;

    drawSpeedView(p, QRect(12, toolbarOffset, w - 24, topWidgetH - toolbarOffset));
    drawCategoryChart(p, QRect(12, topWidgetH + 8, halfW - 16, h - topWidgetH - 20));
    drawStats(p, QRect(halfW + 4, topWidgetH + 8, halfW - 16, h - topWidgetH - 20));
}

void PaperSpeedTracker::drawSpeedView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 4, "Speed Records");

    if (entries_.isEmpty()) return;

    qreal maxWpm = 1.0;
    qreal currentAvg = avgWpm();
    for (const auto& e : entries_) {
        if (e.wpm > maxWpm) maxWpm = e.wpm;
    }

    int maxShow = qMin(10, entries_.size());
    int barAreaH = rect.height() - 10;
    int barH = qMax(16, qMin(30, (barAreaH - (maxShow - 1) * 4) / maxShow));
    int spacing = 4;
    int labelW = qMin(120, rect.width() / 3);
    int barStartX = rect.x() + labelW + 8;
    int barMaxW = rect.width() - labelW - 60;

    int startIdx = qMax(0, entries_.size() - maxShow);
    for (int i = startIdx; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        int row = i - startIdx;
        int y = rect.y() + 14 + row * (barH + spacing);

        // Paper name
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 9));
        QString displayName = entry.paper;
        QFontMetrics fm(p.font());
        if (fm.horizontalAdvance(displayName) > labelW - 4) {
            displayName = fm.elidedText(displayName, Qt::ElideRight, labelW - 4);
        }
        p.drawText(QRect(rect.x(), y, labelW, barH), Qt::AlignVCenter | Qt::AlignLeft, displayName);

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barStartX, y + 2, barMaxW, barH - 4, 3, 3);

        // Bar fill
        qreal ratio = entry.wpm / maxWpm;
        int fillW = static_cast<int>(ratio * barMaxW);
        if (fillW < 4) fillW = 4;

        p.setBrush(entry.color);
        p.drawRoundedRect(barStartX, y + 2, fillW, barH - 4, 3, 3);

        // WPM label on bar
        if (fillW > 40) {
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(QRect(barStartX, y + 2, fillW, barH - 4),
                       Qt::AlignVCenter | Qt::AlignRight,
                       QString::number(static_cast<int>(entry.wpm)) + " wpm");
        }

        // Pages read
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        int pagesX = barStartX + barMaxW + 4;
        p.drawText(QRect(pagesX, y, 48, barH), Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(entry.pagesRead) + "pg");

        // Above average indicator
        if (entry.aboveAverage) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#16a34a"));
            int indicatorR = 4;
            int indX = rect.x() + rect.width() - 12;
            int indY = y + barH / 2;
            QPainterPath indicator;
            indicator.addEllipse(indX - indicatorR, indY - indicatorR,
                                 indicatorR * 2, indicatorR * 2);
            p.drawPath(indicator);
        }
    }

    // Average line
    if (currentAvg > 0 && maxWpm > 0) {
        int avgX = barStartX + static_cast<int>((currentAvg / maxWpm) * barMaxW);
        p.setPen(QPen(QColor(220, 38, 38), 1, Qt::DashLine));
        p.drawLine(avgX, rect.y() + 14, avgX, rect.y() + 14 + maxShow * (barH + spacing));

        p.setPen(QColor(220, 38, 38));
        p.setFont(QFont("Arial", 7));
        p.drawText(avgX + 3, rect.y() + 12, "avg");
    }
}

void PaperSpeedTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 14, "Category Distribution");

    QMap<QString, int> counts = categoryCounts();
    QStringList cats = {"Skimming", "Normal", "Deep", "Speed"};
    int total = 0;
    for (const auto& c : cats) total += counts[c];

    if (total == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 14 + (rect.height() - 14) / 2;
    int outerR = qMin(rect.width(), rect.height() - 30) / 2 - 20;
    int innerR = outerR * 55 / 100;
    if (outerR < 20) outerR = 20;
    if (innerR < 10) innerR = 10;

    qreal startAngle = 0.0;

    for (const auto& cat : cats) {
        int count = counts[cat];
        if (count == 0) continue;

        qreal sweep = (static_cast<qreal>(count) / total) * 360.0;
        QColor color = categoryColor(cat);

        QPainterPath slice;
        slice.moveTo(cx, cy);
        slice.arcTo(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                    startAngle, sweep);
        slice.closeSubpath();

        // Inner cutout for donut
        QPainterPath innerCut;
        innerCut.addEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);
        QPainterPath donutSlice = slice - innerCut;

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(donutSlice);

        // Label line
        qreal midAngle = startAngle + sweep / 2.0;
        qreal rad = qDegreesToRadians(midAngle);
        int labelR = outerR + 10;
        int labelX = cx + static_cast<int>(labelR * std::cos(rad));
        int labelY = cy - static_cast<int>(labelR * std::sin(rad));

        p.setPen(color);
        p.setFont(QFont("Arial", 8));
        p.drawText(labelX - 30, labelY - 6, 60, 14, Qt::AlignCenter,
                   cat + " (" + QString::number(count) + ")");

        startAngle += sweep;
    }

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(QRect(cx - 30, cy - 12, 60, 24), Qt::AlignCenter,
               QString::number(total));
    p.setFont(QFont("Arial", 8));
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(cx - 30, cy + 8, 60, 16), Qt::AlignCenter, "entries");
}

void PaperSpeedTracker::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 14, "Statistics");

    qreal avg = avgWpm();
    int above = aboveAverageCount();
    int totalPages = 0;
    for (const auto& e : entries_) totalPages += e.pagesRead;

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Entries",   QString::number(entries_.size()),          QColor("#3b82f6")},
        {"Total Pages",     QString::number(totalPages),               QColor("#16a34a")},
        {"Average WPM",     QString::number(avg, 'f', 0),              QColor("#7c3aed")},
        {"Above Average",   QString::number(above),                    QColor("#d97706")},
    };

    int boxH = qMin(48, (rect.height() - 30) / stats.size() - 4);
    int boxW = rect.width() - 4;
    int startX = rect.x() + 2;
    int startY = rect.y() + 24;

    for (int i = 0; i < stats.size(); ++i) {
        int y = startY + i * (boxH + 4);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath bgPath;
        bgPath.addRoundedRect(startX, y, boxW, boxH, 6, 6);
        p.drawPath(bgPath);

        // Left accent
        p.setBrush(stats[i].color);
        QPainterPath accent;
        accent.addRoundedRect(startX, y, 4, boxH, 2, 2);
        p.drawPath(accent);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 15, QFont::Bold));
        p.drawText(QRect(startX + 12, y + 2, boxW - 20, boxH * 2 / 3),
                   Qt::AlignVCenter | Qt::AlignLeft, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(startX + 12, y + boxH / 2, boxW - 20, boxH / 2),
                   Qt::AlignVCenter | Qt::AlignLeft, stats[i].label);
    }
}

void PaperSpeedTracker::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Record reading speed entries to begin tracking");
        return;
    }

    qreal avg = avgWpm();
    int above = aboveAverageCount();
    int totalPages = 0;
    for (const auto& e : entries_) totalPages += e.pagesRead;

    infoLabel_->setText(
        QString("%1 entries | Avg: %2 wpm | %3 pages | %4 above avg")
            .arg(entries_.size())
            .arg(avg, 0, 'f', 0)
            .arg(totalPages)
            .arg(above));
}

void PaperSpeedTracker::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("speedEntries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SpeedTrackerEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.method = settings_.value("method").toString();
        e.wpm = settings_.value("wpm").toDouble();
        e.pagesRead = settings_.value("pagesRead").toInt();
        e.aboveAverage = settings_.value("aboveAverage").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSpeedTracker::saveSettings() {
    settings_.beginWriteArray("speedEntries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("paper", e.paper);
        settings_.setValue("category", e.category);
        settings_.setValue("method", e.method);
        settings_.setValue("wpm", e.wpm);
        settings_.setValue("pagesRead", e.pagesRead);
        settings_.setValue("aboveAverage", e.aboveAverage);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}
