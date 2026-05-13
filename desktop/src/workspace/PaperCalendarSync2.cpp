#include "workspace/PaperCalendarSync2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QFontMetrics>
#include <QPaintEvent>
#include <algorithm>
#include <cmath>

namespace {
static const QHash<QString, QColor> kCalendarColors = {
    {"Work",      QColor("#3b82f6")},
    {"Personal",  QColor("#16a34a")},
    {"Academic",  QColor("#d97706")},
    {"Research",  QColor("#dc2626")},
    {"Workshop",  QColor("#7c3aed")}
};

QColor calendarColor(const QString& cal) {
    if (kCalendarColors.contains(cal))
        return kCalendarColors.value(cal);
    return QColor("#64748b");
}

static const QStringList kCalendars = {"Work", "Personal", "Academic", "Research"};
static const QStringList kCategories = {"Meeting", "Seminar", "Deadline", "Review", "Workshop"};
}

PaperCalendarSync2::PaperCalendarSync2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CalendarSync2")
{
    setupUI();
    loadSettings();
}

void PaperCalendarSync2::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Meeting", "Seminar", "Deadline", "Review", "Workshop"});
    categoryCombo_->setMinimumWidth(110);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter event name...");

    syncBtn_ = new QPushButton("Sync", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet("font-weight: bold;");

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(syncBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(infoLabel_);
    toolbar->addStretch();

    mainLayout->addLayout(toolbar);
    mainLayout->addStretch();

    connect(syncBtn_, &QPushButton::clicked, this, &PaperCalendarSync2::onSync);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCalendarSync2::onClear);

    setMinimumHeight(520);
    updateInfo();
}

void PaperCalendarSync2::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int toolbarH = 50;
    int top = toolbarH + 4;
    int drawH = h - top - 8;
    if (drawH < 60) return;

    int topH = drawH * 55 / 100;
    int bottomH = drawH - topH - 6;
    int halfW = (w - 20) / 2;

    drawSyncView(p, QRect(4, top, w - 8, topH));
    drawCategoryChart(p, QRect(4, top + topH + 6, halfW, bottomH));
    drawStats(p, QRect(8 + halfW, top + topH + 6, w - 16 - halfW, bottomH));
}

void PaperCalendarSync2::drawSyncView(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    QPainterPath bg;
    bg.addRoundedRect(rect, 8, 8);
    p.drawPath(bg);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Calendar Sync View");

    int cardTop = rect.top() + 34;
    int cardLeft = rect.left() + 10;
    int cardW = rect.width() - 20;

    if (entries_.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(QRect(cardLeft, cardTop, cardW, 30),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   "No events synced. Click Sync to add.");
        return;
    }

    int cardH = 56;
    int gap = 4;
    QFont nameFont = font();
    nameFont.setPointSize(9);
    nameFont.setBold(true);

    QFont detailFont = font();
    detailFont.setPointSize(8);

    int y = cardTop;
    for (int i = 0; i < entries_.size(); ++i) {
        if (y + cardH > rect.bottom() - 4) break;

        const auto& e = entries_[i];

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        QPainterPath card;
        card.addRoundedRect(QRectF(cardLeft, y, cardW, cardH), 6, 6);
        p.drawPath(card);

        // Card subtle shadow/border
        p.setPen(QPen(QColor(226, 232, 240), 1));
        p.setBrush(Qt::NoBrush);
        QPainterPath cardBorder;
        cardBorder.addRoundedRect(QRectF(cardLeft, y, cardW, cardH), 6, 6);
        p.drawPath(cardBorder);

        // Left color accent bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        QPainterPath accent;
        accent.addRoundedRect(QRectF(cardLeft, y, 5, cardH), 2, 2);
        p.drawPath(accent);

        // Duration bar (horizontal, proportional to max 8h)
        qreal maxDur = 8.0;
        qreal durFrac = std::min(e.duration / maxDur, 1.0);
        int durBarMaxW = 90;
        int durBarW = static_cast<int>(durFrac * durBarMaxW);
        p.setBrush(QColor(e.color.red(), e.color.green(), e.color.blue(), 80));
        QPainterPath durBar;
        durBar.addRoundedRect(QRectF(cardLeft + 14, y + 8, durBarW, 6), 3, 3);
        p.drawPath(durBar);

        // Duration text
        p.setPen(QColor(100, 116, 139));
        p.setFont(detailFont);
        p.drawText(QRect(cardLeft + 14 + durBarW + 4, y + 3, 50, 12),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1h").arg(QString::number(e.duration, 'f', 1)));

        // Event name
        p.setPen(QColor(30, 41, 59));
        p.setFont(nameFont);
        QString displayName = e.event;
        if (displayName.length() > 32)
            displayName = displayName.left(29) + "...";
        p.drawText(QRect(cardLeft + 14, y + 18, cardW - 100, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, displayName);

        // Calendar badge (pill shape)
        QColor calCol = calendarColor(e.calendar);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(calCol.red(), calCol.green(), calCol.blue(), 40));
        int badgeW = 62;
        int badgeX = cardLeft + 14;
        int badgeY = y + 36;
        QPainterPath calBadge;
        calBadge.addRoundedRect(QRectF(badgeX, badgeY, badgeW, 16), 8, 8);
        p.drawPath(calBadge);
        // Badge dot
        p.setBrush(calCol);
        QPainterPath badgeDot;
        badgeDot.addEllipse(QPointF(badgeX + 10, badgeY + 8), 3, 3);
        p.drawPath(badgeDot);
        // Badge text
        p.setPen(calCol);
        p.setFont(detailFont);
        p.drawText(QRect(badgeX + 17, badgeY, badgeW - 17, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, e.calendar);

        // Category tag
        QColor tagBg = QColor(e.color.red(), e.color.green(), e.color.blue(), 40);
        p.setPen(Qt::NoPen);
        p.setBrush(tagBg);
        int tagW = 64;
        int tagX = cardLeft + 84;
        QPainterPath tag;
        tag.addRoundedRect(QRectF(tagX, badgeY, tagW, 16), 8, 8);
        p.drawPath(tag);
        p.setPen(e.color);
        p.setFont(detailFont);
        p.drawText(QRect(tagX, badgeY, tagW, 16),
                   Qt::AlignCenter, e.category);

        // Attendee count (person icon + number)
        int attendX = cardLeft + cardW - 120;
        p.setPen(QColor(100, 116, 139));
        p.setFont(detailFont);

        // Person icon (small circle + body)
        int iconY = y + 8;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(148, 163, 184));
        QPainterPath head;
        head.addEllipse(QPointF(attendX + 5, iconY + 3), 3, 3);
        p.drawPath(head);
        QPainterPath body;
        body.addRoundedRect(QRectF(attendX, iconY + 7, 10, 6), 2, 2);
        p.drawPath(body);

        p.setPen(QColor(100, 116, 139));
        p.drawText(QRect(attendX + 14, iconY, 40, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(e.attendees));

        // Recurring icon (circular arrows) if applicable
        if (e.recurring) {
            int recurX = cardLeft + cardW - 30;
            int recurY = y + 8;
            p.setPen(QPen(QColor("#7c3aed"), 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawArc(recurX, recurY, 14, 14, 30 * 16, 300 * 16);
            // Arrow tip
            QPolygonF arrow;
            arrow << QPointF(recurX + 10, recurY + 1)
                  << QPointF(recurX + 14, recurY + 5)
                  << QPointF(recurX + 7, recurY + 4);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#7c3aed"));
            QPainterPath arrowPath;
            arrowPath.addPolygon(arrow);
            p.drawPath(arrowPath);
        }

        y += cardH + gap;
    }
}

void PaperCalendarSync2::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    QPainterPath bg;
    bg.addRoundedRect(rect, 8, 8);
    p.drawPath(bg);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Calendar Distribution");

    // Aggregate calendar counts
    QMap<QString, int> calCounts;
    for (const auto& e : entries_)
        calCounts[e.calendar]++;

    if (calCounts.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   "No events yet");
        return;
    }

    // Compute total for pie
    int total = 0;
    for (auto it = calCounts.constBegin(); it != calCounts.constEnd(); ++it)
        total += it.value();

    // Pie chart parameters
    int chartTop = rect.top() + 36;
    int availH = rect.height() - 46;
    int pieR = std::min(rect.width() - 20, availH) / 2;
    if (pieR < 30) pieR = 30;
    int cx = rect.left() + pieR + 10;
    int cy = chartTop + pieR;

    // Draw pie slices
    const QColor sliceColors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    int colorIdx = 0;
    qreal startAngle = 0.0;

    QList<QPair<QString, qreal>> legendItems;

    for (auto it = calCounts.constBegin(); it != calCounts.constEnd(); ++it) {
        qreal fraction = static_cast<qreal>(it.value()) / total;
        qreal spanAngle = fraction * 360.0;

        // Slice
        p.setPen(Qt::NoPen);
        QColor col = calendarColor(it.key());
        p.setBrush(col);
        QPainterPath slice;
        slice.moveTo(cx, cy);
        slice.arcTo(cx - pieR, cy - pieR, pieR * 2, pieR * 2,
                    static_cast<int>(startAngle * 16),
                    static_cast<int>(spanAngle * 16));
        slice.closeSubpath();
        p.drawPath(slice);

        legendItems.append({it.key(), fraction * 100.0});
        startAngle += spanAngle;
        ++colorIdx;
    }

    // Center hole (donut style)
    int innerR = pieR * 40 / 100;
    p.setBrush(QColor(248, 250, 252));
    QPainterPath hole;
    hole.addEllipse(QPointF(cx, cy), innerR, innerR);
    p.drawPath(hole);

    // Center total label
    p.setPen(QColor(30, 41, 59));
    QFont centerFont = font();
    centerFont.setBold(true);
    centerFont.setPointSize(12);
    p.setFont(centerFont);
    p.drawText(QRect(cx - innerR, cy - 8, innerR * 2, 16),
               Qt::AlignCenter, QString::number(total));

    // Legend (right side of pie)
    int legendX = cx + pieR + 12;
    int legendY = chartTop + 6;
    int legendW = rect.right() - legendX - 6;
    QFont legendFont = font();
    legendFont.setPointSize(8);
    p.setFont(legendFont);

    colorIdx = 0;
    for (const auto& item : legendItems) {
        if (legendY + 18 > rect.bottom() - 4) break;

        QColor col = calendarColor(item.first);
        // Color dot
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        QPainterPath dot;
        dot.addEllipse(QPointF(legendX + 5, legendY + 7), 4, 4);
        p.drawPath(dot);

        // Name + percentage
        p.setPen(QColor(71, 85, 105));
        p.drawText(QRect(legendX + 14, legendY, legendW - 14, 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1 (%2%)").arg(item.first)
                       .arg(QString::number(item.second, 'f', 0)));

        legendY += 20;
        ++colorIdx;
    }
}

void PaperCalendarSync2::drawStats(QPainter& p, const QRect& rect)
{
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    QPainterPath bg;
    bg.addRoundedRect(rect, 8, 8);
    p.drawPath(bg);

    // Title
    p.setPen(QColor(30, 41, 59));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Statistics");

    int total = entries_.size();
    int recurring = recurringCount();
    qreal avgDur = avgDuration();
    int totalAttendees = 0;
    for (const auto& e : entries_)
        totalAttendees += e.attendees;

    struct StatItem {
        QString label;
        QString value;
        QColor color;
    };

    QList<StatItem> stats = {
        {"Total Events",   QString::number(total),                        QColor("#3b82f6")},
        {"Avg Duration",   QString("%1h").arg(
             QString::number(avgDur, 'f', 1)),                            QColor("#16a34a")},
        {"Recurring",      QString::number(recurring),                     QColor("#d97706")},
        {"Total Attendees", QString::number(totalAttendees),               QColor("#dc2626")}
    };

    int boxW = (rect.width() - 32) / 2;
    int boxH = 52;
    int gap = 8;
    int startX = rect.left() + 10;
    int startY = rect.top() + 36;

    QFont labelFont = font();
    labelFont.setPointSize(7);

    QFont valueFont = font();
    valueFont.setBold(true);
    valueFont.setPointSize(14);

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        int bx = startX + col * (boxW + gap);
        int by = startY + row * (boxH + gap);

        if (bx + boxW > rect.right() - 4 || by + boxH > rect.bottom() - 4) break;

        const auto& s = stats[i];

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        QPainterPath box;
        box.addRoundedRect(QRectF(bx, by, boxW, boxH), 6, 6);
        p.drawPath(box);

        // Box border
        p.setPen(QPen(QColor(226, 232, 240), 1));
        p.setBrush(Qt::NoBrush);
        QPainterPath boxBorder;
        boxBorder.addRoundedRect(QRectF(bx, by, boxW, boxH), 6, 6);
        p.drawPath(boxBorder);

        // Top color accent line
        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        QPainterPath accentLine;
        accentLine.addRoundedRect(QRectF(bx + 6, by + 2, boxW - 12, 3), 1, 1);
        p.drawPath(accentLine);

        // Value
        p.setPen(s.color);
        p.setFont(valueFont);
        p.drawText(QRect(bx + 6, by + 8, boxW - 12, 24),
                   Qt::AlignLeft | Qt::AlignVCenter, s.value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(labelFont);
        p.drawText(QRect(bx + 6, by + 32, boxW - 12, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, s.label);
    }
}

void PaperCalendarSync2::onSync()
{
    QString name = inputField_->text().trimmed();
    if (name.isEmpty())
        name = QString("Event %1").arg(entries_.size() + 1);

    QString category = categoryCombo_->currentText();
    if (category == "All")
        category = kCategories[QRandomGenerator::global()->bounded(kCategories.size())];

    QString calendar = kCalendars[QRandomGenerator::global()->bounded(kCalendars.size())];
    qreal duration = 0.5 + QRandomGenerator::global()->generateDouble() * 7.5;
    int attendees = 1 + QRandomGenerator::global()->bounded(25);
    bool recurring = QRandomGenerator::global()->bounded(3) == 0;

    CalendarSync2Entry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.event = name;
    entry.category = category;
    entry.calendar = calendar;
    entry.duration = std::round(duration * 10.0) / 10.0;
    entry.attendees = attendees;
    entry.recurring = recurring;
    entry.color = calendarColor(calendar);

    entries_.append(entry);
    inputField_->clear();

    saveSettings();
    updateInfo();
    update();

    emit eventSynced(entry.id, entry.duration);
}

void PaperCalendarSync2::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperCalendarSync2::updateInfo()
{
    int n = entries_.size();
    int rec = recurringCount();
    qreal avg = avgDuration();
    infoLabel_->setText(
        QString("Events: %1 | Recurring: %2 | Avg: %3h")
            .arg(n).arg(rec).arg(QString::number(avg, 'f', 1)));
}

void PaperCalendarSync2::loadSettings()
{
    settings_.beginGroup("CalendarSync2");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        CalendarSync2Entry e;
        e.id = settings_.value("id").toInt();
        e.event = settings_.value("event").toString();
        e.category = settings_.value("category").toString();
        e.calendar = settings_.value("calendar").toString();
        e.duration = settings_.value("duration").toReal();
        e.attendees = settings_.value("attendees").toInt();
        e.recurring = settings_.value("recurring").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    // Seed 8 entries on first run
    if (entries_.isEmpty()) {
        static const struct { QString ev; QString cat; QString cal; qreal dur; int att; bool rec; } seeds[] = {
            {"Sprint Planning",        "Meeting",  "Work",      2.0, 8,  true},
            {"ML Paper Discussion",     "Seminar",  "Academic",  1.5, 15, true},
            {"Grant Proposal Due",      "Deadline", "Research",  0.5, 3,  false},
            {"Quarterly Review",        "Review",   "Work",      3.0, 12, true},
            {"Deep Learning Workshop",  "Workshop", "Academic",  4.0, 25, false},
            {"Yoga Session",            "Meeting",  "Personal",  1.0, 5,  true},
            {"Ethics Board Meeting",    "Meeting",  "Research",  1.5, 6,  false},
            {"Journal Club",            "Seminar",  "Academic",  2.0, 10, true}
        };
        for (int i = 0; i < 8; ++i) {
            CalendarSync2Entry e;
            e.id = i + 1;
            e.event = seeds[i].ev;
            e.category = seeds[i].cat;
            e.calendar = seeds[i].cal;
            e.duration = seeds[i].dur;
            e.attendees = seeds[i].att;
            e.recurring = seeds[i].rec;
            e.color = calendarColor(seeds[i].cal);
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
    update();
}

void PaperCalendarSync2::saveSettings()
{
    settings_.beginGroup("CalendarSync2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("event", e.event);
        settings_.setValue("category", e.category);
        settings_.setValue("calendar", e.calendar);
        settings_.setValue("duration", e.duration);
        settings_.setValue("attendees", e.attendees);
        settings_.setValue("recurring", e.recurring);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperCalendarSync2::addEntry(const CalendarSync2Entry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<CalendarSync2Entry> PaperCalendarSync2::entries() const
{
    return entries_;
}

int PaperCalendarSync2::recurringCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.recurring) ++count;
    }
    return count;
}

qreal PaperCalendarSync2::avgDuration() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_)
        total += e.duration;
    return std::round(total / entries_.size() * 10.0) / 10.0;
}

QMap<QString, int> PaperCalendarSync2::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}
