#include "workspace/PaperTeamCalendar.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDate>
#include <QPainterPath>
#include <QFontMetrics>
#include <QPaintEvent>
#include <algorithm>
#include <cmath>

PaperTeamCalendar::PaperTeamCalendar(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TeamCalendar")
{
    setupUI();
    loadSettings();
}

void PaperTeamCalendar::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    auto* leftLayout = new QHBoxLayout();
    leftLayout->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"Meeting", "Deadline", "Review", "Workshop", "Other"});
    categoryCombo_->setMinimumWidth(100);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Event name...");

    scheduleBtn_ = new QPushButton("Schedule", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet("font-weight: bold;");

    leftLayout->addWidget(categoryCombo_);
    leftLayout->addWidget(inputField_);
    leftLayout->addWidget(scheduleBtn_);
    leftLayout->addWidget(clearBtn_);
    leftLayout->addWidget(infoLabel_);

    mainLayout->addLayout(leftLayout);
    mainLayout->addStretch();

    connect(scheduleBtn_, &QPushButton::clicked, this, &PaperTeamCalendar::onSchedule);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTeamCalendar::onClear);

    setMinimumHeight(420);
    updateInfo();
}

void PaperTeamCalendar::loadSettings()
{
    settings_.beginGroup("TeamCalendar");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        CalendarEntry e;
        e.id = settings_.value("id").toInt();
        e.event = settings_.value("event").toString();
        e.category = settings_.value("category").toString();
        e.date = settings_.value("date").toString();
        e.duration = settings_.value("duration").toReal();
        e.attendees = settings_.value("attendees").toInt();
        e.allDay = settings_.value("allDay").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperTeamCalendar::saveSettings()
{
    settings_.beginGroup("TeamCalendar");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("event", e.event);
        settings_.setValue("category", e.category);
        settings_.setValue("date", e.date);
        settings_.setValue("duration", e.duration);
        settings_.setValue("attendees", e.attendees);
        settings_.setValue("allDay", e.allDay);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperTeamCalendar::onSchedule()
{
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) {
        name = QString("Event %1").arg(entries_.size() + 1);
    }

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    qreal duration = 0.5 + QRandomGenerator::global()->generateDouble() * 7.5;
    int attendees = 1 + QRandomGenerator::global()->bounded(20);
    bool allDay = QRandomGenerator::global()->bounded(2) == 0;

    QDate futureDate = QDate::currentDate().addDays(
        QRandomGenerator::global()->bounded(1, 366));
    QString dateStr = futureDate.toString("yyyy-MM-dd");

    CalendarEntry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.event = name;
    entry.category = categoryCombo_->currentText();
    entry.date = dateStr;
    entry.duration = std::round(duration * 10.0) / 10.0;
    entry.attendees = attendees;
    entry.allDay = allDay;
    entry.color = palette[QRandomGenerator::global()->bounded(5)];

    entries_.append(entry);
    inputField_->clear();

    saveSettings();
    updateInfo();
    update();

    emit eventScheduled(entry.id, entry.duration);
}

void PaperTeamCalendar::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
    repaint();
}

void PaperTeamCalendar::addEntry(const CalendarEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<CalendarEntry> PaperTeamCalendar::entries() const
{
    return entries_;
}

int PaperTeamCalendar::allDayCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.allDay) ++count;
    }
    return count;
}

qreal PaperTeamCalendar::avgDuration() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) {
        total += e.duration;
    }
    return total / entries_.size();
}

QMap<QString, int> PaperTeamCalendar::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperTeamCalendar::updateInfo()
{
    int n = entries_.size();
    int ad = allDayCount();
    qreal avg = avgDuration();
    infoLabel_->setText(
        QString("Events: %1 | All-Day: %2 | Avg: %3h")
            .arg(n).arg(ad).arg(QString::number(avg, 'f', 1)));
}

void PaperTeamCalendar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int toolbarH = 50;
    int top = toolbarH + 4;
    int drawH = h - top - 8;
    if (drawH < 50) return;

    int colW = w / 3;

    drawCalendarView(p, QRect(4, top, colW - 8, drawH));
    drawCategoryChart(p, QRect(colW + 4, top, colW - 8, drawH));
    drawStats(p, QRect(2 * colW + 4, top, colW - 12, drawH));
}

void PaperTeamCalendar::drawCalendarView(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Team Calendar");

    QDate today = QDate::currentDate();
    QDate monthStart = QDate(today.year(), today.month(), 1);
    int daysInMonth = monthStart.daysInMonth();
    int startDow = monthStart.dayOfWeek(); // 1=Mon .. 7=Sun

    int gridTop = rect.top() + 32;
    int gridLeft = rect.left() + 8;
    int gridW = rect.width() - 16;
    int gridH = rect.bottom() - gridTop - 8;
    if (gridH < 20) return;

    // Day headers
    QFont smallFont = font();
    smallFont.setPointSize(7);
    p.setFont(smallFont);
    p.setPen(QColor(100, 116, 139));

    QStringList headers = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
    qreal cellW = gridW / 7.0;
    qreal headerH = 16;
    for (int i = 0; i < 7; ++i) {
        p.drawText(QRectF(gridLeft + i * cellW, gridTop, cellW, headerH),
                   Qt::AlignCenter, headers[i]);
    }

    // Compute rows needed
    int totalCells = startDow - 1 + daysInMonth;
    int rows = (totalCells + 6) / 7;
    qreal rowH = (gridH - headerH) / rows;

    // Day numbers
    p.setFont(smallFont);
    for (int d = 1; d <= daysInMonth; ++d) {
        int cellIdx = startDow - 1 + d - 1;
        int row = cellIdx / 7;
        int col = cellIdx % 7;
        qreal x = gridLeft + col * cellW;
        qreal y = gridTop + headerH + row * rowH;

        // Highlight today
        if (d == today.day()) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#3b82f6"));
            p.drawEllipse(QPointF(x + cellW / 2, y + rowH / 2), 10, 10);
            p.setPen(Qt::white);
        } else {
            p.setPen(QColor(71, 85, 105));
        }
        p.drawText(QRectF(x, y, cellW, rowH), Qt::AlignCenter, QString::number(d));
    }

    // Event dots and all-day spans
    for (const auto& e : entries_) {
        QDate evDate = QDate::fromString(e.date, "yyyy-MM-dd");
        if (!evDate.isValid() || evDate.month() != today.month()
            || evDate.year() != today.year())
            continue;

        int d = evDate.day();
        int cellIdx = startDow - 1 + d - 1;
        int row = cellIdx / 7;
        int col = cellIdx % 7;
        qreal x = gridLeft + col * cellW;
        qreal y = gridTop + headerH + row * rowH;

        if (e.allDay) {
            // All-day spans full row width
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(e.color.red(), e.color.green(), e.color.blue(), 60));
            p.drawRoundedRect(QRectF(gridLeft, y + rowH - 10, gridW, 8), 3, 3);
        } else {
            // Small dot below the day number
            p.setPen(Qt::NoPen);
            p.setBrush(e.color);
            p.drawEllipse(QPointF(x + cellW / 2, y + rowH - 6), 3, 3);
        }
    }
}

void PaperTeamCalendar::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   "No events yet");
        return;
    }

    static const QHash<QString, QColor> catColors = {
        {"Meeting", QColor("#3b82f6")},
        {"Deadline", QColor("#dc2626")},
        {"Review",   QColor("#16a34a")},
        {"Workshop", QColor("#d97706")},
        {"Other",    QColor("#7c3aed")}
    };

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount) maxCount = it.value();
    }

    int chartTop = rect.top() + 34;
    int chartLeft = rect.left() + 10;
    int chartW = rect.width() - 20;
    int barH = 22;
    int gap = 6;

    QFont labelFont = font();
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    int y = chartTop;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + barH > rect.bottom() - 4) break;

        // Category label
        p.setPen(QColor(71, 85, 105));
        p.drawText(QRect(chartLeft, y, 60, barH), Qt::AlignLeft | Qt::AlignVCenter,
                   it.key());

        // Bar
        qreal barFrac = maxCount > 0 ? static_cast<qreal>(it.value()) / maxCount : 0.0;
        int barStartX = chartLeft + 64;
        int maxBarW = chartW - 100;
        int barW = static_cast<int>(maxBarW * barFrac);

        QColor col = catColors.value(it.key(), QColor("#3b82f6"));
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawRoundedRect(QRect(barStartX, y + 3, barW, barH - 6), 3, 3);

        // Count label
        p.setPen(QColor(30, 41, 59));
        p.drawText(QRect(barStartX + barW + 4, y, 30, barH),
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += barH + gap;
    }
}

void PaperTeamCalendar::drawStats(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statFont = font();
    statFont.setPointSize(10);
    p.setFont(statFont);

    int totalEvents = entries_.size();
    int allDay = allDayCount();
    qreal avgDur = avgDuration();

    struct StatItem {
        QString label;
        QString value;
        QColor color;
    };

    QList<StatItem> stats = {
        {"Total Events", QString::number(totalEvents), QColor("#3b82f6")},
        {"All-Day Count", QString::number(allDay),     QColor("#16a34a")},
        {"Avg Duration",  QString("%1h").arg(QString::number(avgDur, 'f', 1)),
                                                    QColor("#d97706")},
    };

    int y = rect.top() + 40;
    int itemH = 44;
    int spacing = 10;
    int leftMargin = rect.left() + 14;

    for (const auto& s : stats) {
        if (y + itemH > rect.bottom() - 4) break;

        // Color indicator circle
        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        p.drawEllipse(QPointF(leftMargin + 8, y + itemH / 2), 6, 6);

        // Label
        p.setPen(QColor(100, 116, 139));
        QFont lblFont = font();
        lblFont.setPointSize(8);
        p.setFont(lblFont);
        p.drawText(QRect(leftMargin + 22, y, rect.width() - 40, itemH / 2),
                   Qt::AlignLeft | Qt::AlignBottom, s.label);

        // Value
        p.setPen(QColor(30, 41, 59));
        QFont valFont = font();
        valFont.setBold(true);
        valFont.setPointSize(13);
        p.setFont(valFont);
        p.drawText(QRect(leftMargin + 22, y + itemH / 2, rect.width() - 40, itemH / 2),
                   Qt::AlignLeft | Qt::AlignTop, s.value);

        y += itemH + spacing;
    }
}
