#include "reading/ReadingSpeedAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDateEdit>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

ReadingSpeedAnalyzer::ReadingSpeedAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingSpeed")
{
    setupUI();
    loadSettings();
}

void ReadingSpeedAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"7 Days", "30 Days", "90 Days", "All"});
    connect(periodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingSpeedAnalyzer::onPeriodChanged);
    toolbar->addWidget(periodCombo_, 1);

    addBtn_ = new QPushButton("Log Reading");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &ReadingSpeedAnalyzer::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &ReadingSpeedAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Log reading sessions to track speed");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(500, 380);
}

void ReadingSpeedAnalyzer::addRecord(const SpeedRecord& record) {
    records_.append(record);
    saveSettings();
    updateInfo();
    update();
    if (record.minutesSpent > 0) {
        emit recordAdded(record.date, static_cast<qreal>(record.wordsRead) / record.minutesSpent);
    }
}

QList<SpeedRecord> ReadingSpeedAnalyzer::records() const { return records_; }

qreal ReadingSpeedAnalyzer::averageWPM() const {
    int totalW = 0, totalM = 0;
    for (const auto& r : records_) { totalW += r.wordsRead; totalM += r.minutesSpent; }
    return totalM > 0 ? static_cast<qreal>(totalW) / totalM : 0;
}

qreal ReadingSpeedAnalyzer::averageWPMThisWeek() const {
    int totalW = 0, totalM = 0;
    QDate weekStart = QDate::currentDate().addDays(-7);
    for (const auto& r : records_) {
        if (r.date >= weekStart) { totalW += r.wordsRead; totalM += r.minutesSpent; }
    }
    return totalM > 0 ? static_cast<qreal>(totalW) / totalM : 0;
}

int ReadingSpeedAnalyzer::totalWordsRead() const {
    int t = 0; for (const auto& r : records_) t += r.wordsRead; return t;
}

int ReadingSpeedAnalyzer::totalMinutes() const {
    int t = 0; for (const auto& r : records_) t += r.minutesSpent; return t;
}

void ReadingSpeedAnalyzer::onAdd() {
    bool ok;
    int words = QInputDialog::getInt(this, "Log Reading", "Words read:", 2000, 0, 100000, 100, &ok);
    if (!ok) return;
    int mins = QInputDialog::getInt(this, "Log Reading", "Minutes spent:", 30, 1, 480, 5, &ok);
    if (!ok) return;
    SpeedRecord r;
    r.date = QDate::currentDate();
    r.wordsRead = words;
    r.minutesSpent = mins;
    addRecord(r);
}

void ReadingSpeedAnalyzer::onClear() {
    records_.clear();
    saveSettings();
    updateInfo();
    update();
}

void ReadingSpeedAnalyzer::onPeriodChanged(int) { update(); }

void ReadingSpeedAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (records_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Log reading sessions to track speed");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 50, "Reading Speed Analysis");

    int w = width(), h = height();
    drawSpeedChart(p, QRect(20, 70, w - 40, h / 2 - 50));
    drawDistribution(p, QRect(20, h / 2 + 10, w / 2 - 30, h / 2 - 50));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 30, h / 2 - 50));
}

void ReadingSpeedAnalyzer::drawSpeedChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Words Per Minute Over Time");

    // Group by date
    QMap<QDate, qreal> dailyWPM;
    QMap<QDate, int> dailyWords, dailyMins;
    for (const auto& r : records_) {
        dailyWords[r.date] += r.wordsRead;
        dailyMins[r.date] += r.minutesSpent;
    }
    for (auto it = dailyWords.begin(); it != dailyWords.end(); ++it) {
        if (dailyMins[it.key()] > 0)
            dailyWPM[it.key()] = static_cast<qreal>(it.value()) / dailyMins[it.key()];
    }

    if (dailyWPM.isEmpty()) return;
    QList<QDate> dates = dailyWPM.keys();
    std::sort(dates.begin(), dates.end());
    qreal maxWPM = 1;
    for (const auto& v : dailyWPM) maxWPM = qMax(maxWPM, v);

    int chartY = rect.y() + 18;
    int chartH = rect.height() - 25;

    // Grid
    p.setPen(QPen(QColor(241, 245, 249), 1));
    for (int i = 0; i <= 4; ++i) {
        int y = chartY + chartH * i / 4;
        p.drawLine(rect.x(), y, rect.right(), y);
    }

    QPolygonF points;
    for (int i = 0; i < dates.size(); ++i) {
        qreal x = rect.x() + (static_cast<qreal>(i) / qMax(1, dates.size() - 1)) * rect.width();
        qreal y = chartY + chartH - (dailyWPM[dates[i]] / maxWPM) * chartH;
        points << QPointF(x, y);
    }

    // Area
    QPolygonF area = points;
    area << QPointF(points.last().x(), chartY + chartH);
    area << QPointF(points.first().x(), chartY + chartH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(16, 185, 129, 30));
    p.drawPolygon(area);

    p.setPen(QPen(QColor(16, 185, 129), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(points);

    p.setBrush(QColor(16, 185, 129));
    for (const auto& pt : points) p.drawEllipse(pt, 3, 3);
}

void ReadingSpeedAnalyzer::drawDistribution(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Speed Distribution");

    // Bucket: <50, 50-100, 100-150, 150-200, 200-300, 300+
    QMap<QString, int> buckets = {{"<50", 0}, {"50-100", 0}, {"100-150", 0}, {"150-200", 0}, {"200-300", 0}, {"300+", 0}};
    for (const auto& r : records_) {
        if (r.minutesSpent <= 0) continue;
        qreal wpm = static_cast<qreal>(r.wordsRead) / r.minutesSpent;
        if (wpm < 50) buckets["<50"]++;
        else if (wpm < 100) buckets["50-100"]++;
        else if (wpm < 150) buckets["100-150"]++;
        else if (wpm < 200) buckets["150-200"]++;
        else if (wpm < 300) buckets["200-300"]++;
        else buckets["300+"]++;
    }

    int maxVal = 1;
    for (const auto& v : buckets) maxVal = qMax(maxVal, v);

    int barW = (rect.width() - 20) / buckets.size();
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(234,179,8),
                       QColor(16,185,129), QColor(59,130,246), QColor(139,92,246)};

    int i = 0;
    for (auto it = buckets.begin(); it != buckets.end(); ++it, ++i) {
        int x = rect.x() + 10 + i * barW;
        qreal h = (static_cast<qreal>(it.value()) / maxVal) * (rect.height() - 50);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i % 6]);
        p.drawRoundedRect(x, rect.bottom() - 30 - static_cast<int>(h), barW - 6, static_cast<int>(h), 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(x, rect.bottom() - 12, barW - 6, 14, Qt::AlignCenter, it.key());
    }
}

void ReadingSpeedAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Avg WPM", QString::number(averageWPM(), 'f', 0), QColor(59,130,246)},
        {"This Week", QString::number(averageWPMThisWeek(), 'f', 0), QColor(16,185,129)},
        {"Total Words", QString::number(totalWordsRead()), QColor(245,158,11)},
        {"Total Hours", QString::number(totalMinutes() / 60.0, 'f', 1), QColor(139,92,246)}
    };

    int boxH = qMin(45, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 24, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void ReadingSpeedAnalyzer::updateInfo() {
    if (records_.isEmpty()) { infoLabel_->setText("Log reading sessions to track speed"); return; }
    infoLabel_->setText(QString("Avg: %1 WPM | This week: %2 WPM | %3 sessions logged")
        .arg(averageWPM(), 0, 'f', 0).arg(averageWPMThisWeek(), 0, 'f', 0).arg(records_.size()));
}

void ReadingSpeedAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("records");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SpeedRecord r;
        r.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        r.wordsRead = settings_.value("wordsRead").toInt();
        r.minutesSpent = settings_.value("minutesSpent").toInt();
        r.paperId = settings_.value("paperId").toInt();
        r.paperTitle = settings_.value("paperTitle").toString();
        records_.append(r);
    }
    settings_.endArray();
    updateInfo();
}

void ReadingSpeedAnalyzer::saveSettings() {
    settings_.beginWriteArray("records");
    for (int i = 0; i < records_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("date", records_[i].date.toString(Qt::ISODate));
        settings_.setValue("wordsRead", records_[i].wordsRead);
        settings_.setValue("minutesSpent", records_[i].minutesSpent);
        settings_.setValue("paperId", records_[i].paperId);
        settings_.setValue("paperTitle", records_[i].paperTitle);
    }
    settings_.endArray();
}
