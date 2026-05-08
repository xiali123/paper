#include "reading/PaperReadingHeatmap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <cmath>

PaperReadingHeatmap::PaperReadingHeatmap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingHeatmap")
{
    setupUI();
    loadSettings();
}

void PaperReadingHeatmap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Year:"));
    yearCombo_ = new QComboBox();
    yearCombo_->addItems({"2024", "2025", "2026", "2027"});
    yearCombo_->setCurrentIndex(2);
    displayYear_ = 2026;
    connect(yearCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperReadingHeatmap::onYearChanged);
    toolbar->addWidget(yearCombo_);

    toolbar->addStretch();
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingHeatmap::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track reading activity on heatmap");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 450);
}

void PaperReadingHeatmap::addReadingDay(const QDate& date, int pagesRead) {
    readingDays_[date] += pagesRead;
    rebuildCells();
    saveSettings();
    updateInfo();
    emit statsUpdated(totalDaysRead(), currentStreak());
    update();
}

QList<HeatmapCell> PaperReadingHeatmap::cells() const { return cells_; }

int PaperReadingHeatmap::totalDaysRead() const {
    int count = 0;
    for (auto it = readingDays_.begin(); it != readingDays_.end(); ++it) {
        if (it.key().year() == displayYear_) count++;
    }
    return count;
}

int PaperReadingHeatmap::longestStreak() const {
    QList<QDate> dates;
    for (auto it = readingDays_.begin(); it != readingDays_.end(); ++it) {
        if (it.key().year() == displayYear_) dates.append(it.key());
    }
    std::sort(dates.begin(), dates.end());
    if (dates.isEmpty()) return 0;

    int maxStreak = 1, streak = 1;
    for (int i = 1; i < dates.size(); ++i) {
        if (dates[i].toJulianDay() - dates[i-1].toJulianDay() == 1) {
            streak++;
            maxStreak = qMax(maxStreak, streak);
        } else {
            streak = 1;
        }
    }
    return maxStreak;
}

int PaperReadingHeatmap::currentStreak() const {
    QDate today = QDate::currentDate();
    int streak = 0;
    QDate d = today;
    while (readingDays_.contains(d)) {
        streak++;
        d = d.addDays(-1);
    }
    return streak;
}

int PaperReadingHeatmap::totalPages() const {
    int total = 0;
    for (auto it = readingDays_.begin(); it != readingDays_.end(); ++it) {
        if (it.key().year() == displayYear_) total += it.value();
    }
    return total;
}

void PaperReadingHeatmap::onYearChanged(int index) {
    displayYear_ = 2024 + index;
    rebuildCells();
    updateInfo();
    update();
}

void PaperReadingHeatmap::onClear() {
    readingDays_.clear();
    cells_.clear();
    saveSettings();
    infoLabel_->setText("Track reading activity on heatmap");
    update();
}

void PaperReadingHeatmap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (readingDays_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading activity on heatmap");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, QString("Reading Heatmap %1").arg(displayYear_));

    int w = width(), h = height();
    drawHeatmap(p, QRect(20, 50, w - 40, h / 2 - 20));
    drawLegend(p, QRect(20, h / 2 + 30, w / 2 - 30, 30));
    drawStats(p, QRect(20, h / 2 + 65, w / 2 - 30, h / 2 - 90));
    drawMonthChart(p, QRect(w / 2 + 10, h / 2 + 30, w / 2 - 30, h / 2 - 50));
}

void PaperReadingHeatmap::drawHeatmap(QPainter& p, const QRect& rect) {
    QStringList months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    for (int m = 0; m < 12; ++m) {
        int x = rect.x() + 35 + m * ((rect.width() - 40) / 12);
        p.drawText(x, rect.y() + 10, months[m]);
    }

    int cellSize = qMin(14, (rect.width() - 45) / 53);
    int maxCount = 1;
    for (auto it = readingDays_.begin(); it != readingDays_.end(); ++it) {
        if (it.key().year() == displayYear_) maxCount = qMax(maxCount, it.value());
    }

    QDate start(displayYear_, 1, 1);
    QDate end(displayYear_, 12, 31);

    for (QDate d = start; d <= end; d = d.addDays(1)) {
        int week = (d.dayOfYear() - 1) / 7;
        int dayOfWeek = d.dayOfWeek() - 1;

        int x = rect.x() + 35 + week * (cellSize + 1);
        int y = rect.y() + 20 + dayOfWeek * (cellSize + 1);

        int count = readingDays_.value(d, 0);
        QColor color;
        if (count == 0) color = QColor(241, 245, 249);
        else if (count <= maxCount * 0.25) color = QColor(16, 185, 129, 80);
        else if (count <= maxCount * 0.5) color = QColor(16, 185, 129, 140);
        else if (count <= maxCount * 0.75) color = QColor(16, 185, 129, 200);
        else color = QColor(16, 185, 129);

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(x, y, cellSize, cellSize, 2, 2);
    }
}

void PaperReadingHeatmap::drawLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x(), rect.y() + 12, "Less");

    QColor levels[] = {QColor(241,245,249), QColor(16,185,129,80), QColor(16,185,129,140),
                       QColor(16,185,129,200), QColor(16,185,129)};
    for (int i = 0; i < 5; ++i) {
        p.setPen(Qt::NoPen);
        p.setBrush(levels[i]);
        p.drawRoundedRect(rect.x() + 30 + i * 16, rect.y() + 2, 12, 12, 2, 2);
    }

    p.setPen(QColor(100, 116, 139));
    p.drawText(rect.x() + 115, rect.y() + 12, "More");
}

void PaperReadingHeatmap::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Days Read", QString::number(totalDaysRead()), QColor(16,185,129)},
        {"Pages", QString::number(totalPages()), QColor(59,130,246)},
        {"Best Streak", QString::number(longestStreak()), QColor(245,158,11)},
        {"Current", QString::number(currentStreak()), QColor(139,92,246)}
    };

    int boxW = qMin(100, (rect.width() - 15) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, rect.y(), boxW, 42, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 6, rect.y() + 4, boxW - 12, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 6, rect.y() + 26, boxW - 12, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingHeatmap::drawMonthChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Monthly Activity");

    QMap<int, int> monthly;
    for (auto it = readingDays_.begin(); it != readingDays_.end(); ++it) {
        if (it.key().year() == displayYear_) monthly[it.key().month()] += it.value();
    }

    int maxVal = 1;
    for (const auto& v : monthly) maxVal = qMax(maxVal, v);

    int barW = qMin(35, (rect.width() - 20) / 12);
    for (int m = 1; m <= 12; ++m) {
        int x = rect.x() + 5 + (m - 1) * barW;
        int count = monthly.value(m, 0);
        qreal h = (static_cast<qreal>(count) / maxVal) * (rect.height() - 45);

        p.setPen(Qt::NoPen);
        p.setBrush(count > 0 ? QColor(16, 185, 129) : QColor(241, 245, 249));
        p.drawRoundedRect(x, rect.bottom() - 25 - static_cast<int>(h), barW - 3, static_cast<int>(h), 2, 2);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 1, rect.bottom() - 8, barW, 12, Qt::AlignCenter,
                   QString::number(m));
    }
}

void PaperReadingHeatmap::updateInfo() {
    if (readingDays_.isEmpty()) { infoLabel_->setText("Track reading activity on heatmap"); return; }
    infoLabel_->setText(QString("%1 days | %2 pages | streak: %3 | best: %4")
        .arg(totalDaysRead()).arg(totalPages()).arg(currentStreak()).arg(longestStreak()));
}

void PaperReadingHeatmap::rebuildCells() {
    cells_.clear();
    QDate start(displayYear_, 1, 1);
    QDate end(displayYear_, 12, 31);
    int maxCount = 1;
    for (auto it = readingDays_.begin(); it != readingDays_.end(); ++it) {
        if (it.key().year() == displayYear_) maxCount = qMax(maxCount, it.value());
    }
    for (QDate d = start; d <= end; d = d.addDays(1)) {
        HeatmapCell c;
        c.date = d;
        c.count = readingDays_.value(d, 0);
        c.intensity = maxCount > 0 ? static_cast<qreal>(c.count) / maxCount : 0;
        cells_.append(c);
    }
}

void PaperReadingHeatmap::loadSettings() {
    int size = settings_.beginReadArray("days");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        QDate d = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        int count = settings_.value("pages").toInt();
        readingDays_[d] = count;
    }
    settings_.endArray();
    rebuildCells();
    updateInfo();
}

void PaperReadingHeatmap::saveSettings() {
    settings_.beginWriteArray("days");
    int i = 0;
    for (auto it = readingDays_.begin(); it != readingDays_.end(); ++it, ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("date", it.key().toString(Qt::ISODate));
        settings_.setValue("pages", it.value());
    }
    settings_.endArray();
}
