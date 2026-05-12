#include "workspace/PaperCalendarSync.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QFontMetrics>
#include <QPaintEvent>
#include <algorithm>
#include <cmath>

namespace {
static const QHash<QString, QColor> kCategoryColors = {
    {"Meeting",  QColor("#3b82f6")},
    {"Deadline", QColor("#16a34a")},
    {"Reminder", QColor("#7c3aed")},
    {"Review",   QColor("#d97706")}
};

QColor categoryColor(const QString& cat) {
    if (kCategoryColors.contains(cat))
        return kCategoryColors.value(cat);
    return QColor("#64748b");
}
}

PaperCalendarSync::PaperCalendarSync(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CalendarSync")
{
    setupUI();
    loadSettings();
}

void PaperCalendarSync::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Meeting", "Deadline", "Reminder", "Review"});
    categoryCombo_->setMinimumWidth(110);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter event...");

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

    connect(syncBtn_, &QPushButton::clicked, this, &PaperCalendarSync::onSync);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCalendarSync::onClear);

    setMinimumHeight(480);
    updateInfo();
}

void PaperCalendarSync::paintEvent(QPaintEvent*)
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

    drawCalendarView(p, QRect(4, top, w - 8, topH));
    drawCategoryChart(p, QRect(4, top + topH + 6, halfW, bottomH));
    drawStats(p, QRect(8 + halfW, top + topH + 6, w - 16 - halfW, bottomH));
}

void PaperCalendarSync::drawCalendarView(QPainter& p, const QRect& rect)
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

    int cardH = 52;
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

        // Left color accent bar
        p.setBrush(e.color);
        QPainterPath accent;
        accent.addRoundedRect(QRectF(cardLeft, y, 5, cardH), 2, 2);
        p.drawPath(accent);

        // Duration bar (horizontal, proportional to max 8h)
        qreal maxDur = 8.0;
        qreal durFrac = std::min(e.duration / maxDur, 1.0);
        int durBarW = static_cast<int>(durFrac * 80);
        p.setBrush(QColor(e.color.red(), e.color.green(), e.color.blue(), 80));
        QPainterPath durBar;
        durBar.addRoundedRect(QRectF(cardLeft + 12, y + 6, durBarW, 6), 3, 3);
        p.drawPath(durBar);

        // Duration text
        p.setPen(QColor(100, 116, 139));
        p.setFont(detailFont);
        p.drawText(QRect(cardLeft + 12 + durBarW + 4, y + 2, 60, 10),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1h").arg(QString::number(e.duration, 'f', 1)));

        // Event name
        p.setPen(QColor(30, 41, 59));
        p.setFont(nameFont);
        QString displayName = e.event;
        if (displayName.length() > 36) {
            displayName = displayName.left(33) + "...";
        }
        p.drawText(QRect(cardLeft + 12, y + 16, cardW - 80, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, displayName);

        // Attendees count
        p.setPen(QColor(100, 116, 139));
        p.setFont(detailFont);
        p.drawText(QRect(cardLeft + 12, y + 32, 80, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(e.attendees) + " attendees");

        // Calendar label
        p.setPen(QColor(71, 85, 105));
        p.drawText(QRect(cardLeft + 100, y + 32, 80, 14),
                   Qt::AlignLeft | Qt::AlignVCenter, e.calendar);

        // Category tag
        QColor tagBg = QColor(e.color.red(), e.color.green(), e.color.blue(), 40);
        p.setPen(Qt::NoPen);
        p.setBrush(tagBg);
        int tagW = 60;
        int tagX = cardLeft + cardW - tagW - 40;
        QPainterPath tag;
        tag.addRoundedRect(QRectF(tagX, y + 32, tagW, 14), 4, 4);
        p.drawPath(tag);
        p.setPen(e.color);
        p.setFont(detailFont);
        p.drawText(QRect(tagX, y + 32, tagW, 14),
                   Qt::AlignCenter, e.category);

        // Synced checkmark
        if (e.synced) {
            int checkX = cardLeft + cardW - 24;
            int checkY = y + 8;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#16a34a"));
            QPainterPath checkCircle;
            checkCircle.addEllipse(QPointF(checkX + 8, checkY + 8), 8, 8);
            p.drawPath(checkCircle);

            // Draw checkmark
            p.setPen(QPen(Qt::white, 2));
            p.drawLine(checkX + 4, checkY + 8, checkX + 7, checkY + 11);
            p.drawLine(checkX + 7, checkY + 11, checkX + 13, checkY + 5);
        }

        y += cardH + gap;
    }
}

void PaperCalendarSync::drawCategoryChart(QPainter& p, const QRect& rect)
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
               "Categories");

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
        p.drawText(QRect(chartLeft, y, 60, barH),
                   Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar
        qreal barFrac = maxCount > 0
            ? static_cast<qreal>(it.value()) / maxCount : 0.0;
        int barStartX = chartLeft + 64;
        int maxBarW = chartW - 100;
        int barW = static_cast<int>(maxBarW * barFrac);

        QColor col = categoryColor(it.key());
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        QPainterPath bar;
        bar.addRoundedRect(QRectF(barStartX, y + 3, barW, barH - 6), 3, 3);
        p.drawPath(bar);

        // Count label
        p.setPen(QColor(30, 41, 59));
        p.drawText(QRect(barStartX + barW + 4, y, 30, barH),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(it.value()));

        y += barH + gap;
    }
}

void PaperCalendarSync::drawStats(QPainter& p, const QRect& rect)
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
    int synced = syncedCount();
    qreal dur = totalDuration();

    struct StatItem {
        QString label;
        QString value;
        QColor color;
    };

    QList<StatItem> stats = {
        {"Total Events",  QString::number(total),          QColor("#3b82f6")},
        {"Total Duration", QString("%1h").arg(
             QString::number(dur, 'f', 1)),                QColor("#16a34a")},
        {"Synced",        QString::number(synced),          QColor("#7c3aed")},
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
        QPainterPath dot;
        dot.addEllipse(QPointF(leftMargin + 8, y + itemH / 2.0), 6, 6);
        p.drawPath(dot);

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
        p.drawText(QRect(leftMargin + 22, y + itemH / 2,
                         rect.width() - 40, itemH / 2),
                   Qt::AlignLeft | Qt::AlignTop, s.value);

        y += itemH + spacing;
    }
}

void PaperCalendarSync::onSync()
{
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) {
        name = QString("Event %1").arg(entries_.size() + 1);
    }

    static const QStringList calendars = {
        "Google", "Outlook", "Apple", "Local"
    };

    qreal duration = 0.5 + QRandomGenerator::global()->generateDouble() * 7.5;
    int attendees = 1 + QRandomGenerator::global()->bounded(25);

    QString category = categoryCombo_->currentText();
    if (category == "All") {
        static const QStringList cats = {"Meeting", "Deadline", "Reminder", "Review"};
        category = cats[QRandomGenerator::global()->bounded(cats.size())];
    }

    CalendarSyncEntry entry;
    entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    entry.event = name;
    entry.category = category;
    entry.calendar = calendars[QRandomGenerator::global()->bounded(calendars.size())];
    entry.duration = std::round(duration * 10.0) / 10.0;
    entry.attendees = attendees;
    entry.synced = true;
    entry.color = categoryColor(category);

    entries_.append(entry);
    inputField_->clear();

    saveSettings();
    updateInfo();
    update();

    emit eventSynced(entry.id, entry.duration);
}

void PaperCalendarSync::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperCalendarSync::updateInfo()
{
    int n = entries_.size();
    int synced = syncedCount();
    qreal dur = totalDuration();
    infoLabel_->setText(
        QString("Events: %1 | Synced: %2 | Duration: %3h")
            .arg(n).arg(synced).arg(QString::number(dur, 'f', 1)));
}

void PaperCalendarSync::loadSettings()
{
    settings_.beginGroup("CalendarSync");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        CalendarSyncEntry e;
        e.id = settings_.value("id").toInt();
        e.event = settings_.value("event").toString();
        e.category = settings_.value("category").toString();
        e.calendar = settings_.value("calendar").toString();
        e.duration = settings_.value("duration").toReal();
        e.attendees = settings_.value("attendees").toInt();
        e.synced = settings_.value("synced").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperCalendarSync::saveSettings()
{
    settings_.beginGroup("CalendarSync");
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
        settings_.setValue("synced", e.synced);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperCalendarSync::addEntry(const CalendarSyncEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<CalendarSyncEntry> PaperCalendarSync::entries() const
{
    return entries_;
}

int PaperCalendarSync::syncedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.synced) ++count;
    }
    return count;
}

qreal PaperCalendarSync::totalDuration() const
{
    qreal total = 0.0;
    for (const auto& e : entries_) {
        total += e.duration;
    }
    return std::round(total * 10.0) / 10.0;
}

QMap<QString, int> PaperCalendarSync::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}
