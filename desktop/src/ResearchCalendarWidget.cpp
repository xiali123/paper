#include "ResearchCalendarWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

ResearchCalendarWidget::ResearchCalendarWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ResearchCalendar")
{
    setupUI();
    loadSettings();
}

void ResearchCalendarWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Month:"));
    monthCombo_ = new QComboBox();
    QStringList months;
    for (int m = 1; m <= 12; ++m) {
        QDate d(2026, m, 1);
        months << d.toString("MMM yyyy");
    }
    monthCombo_->addItems(months);
    displayMonth_ = QDate::currentDate().month();
    displayYear_ = QDate::currentDate().year();
    monthCombo_->setCurrentIndex(displayMonth_ - 1);
    connect(monthCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ResearchCalendarWidget::onMonthChanged);
    toolbar->addWidget(monthCombo_, 1);

    addBtn_ = new QPushButton("Add Event");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &ResearchCalendarWidget::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &ResearchCalendarWidget::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add research events to calendar");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void ResearchCalendarWidget::addEvent(const CalendarEvent& event) {
    events_.append(event);
    saveSettings();
    updateInfo();
    emit calendarUpdated(events_.size());
    update();
}

QList<CalendarEvent> ResearchCalendarWidget::events() const { return events_; }

QList<CalendarEvent> ResearchCalendarWidget::eventsForDate(const QDate& date) const {
    QList<CalendarEvent> result;
    for (const auto& e : events_) {
        if (e.date == date) result.append(e);
    }
    return result;
}

QMap<QString, int> ResearchCalendarWidget::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : events_) counts[e.category]++;
    return counts;
}

void ResearchCalendarWidget::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Event", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QStringList cats = {"reading", "deadline", "meeting", "review", "submission"};
    QString cat = QInputDialog::getItem(this, "Add Event", "Category:", cats, 0, false, &ok);
    if (!ok) return;
    int day = QInputDialog::getInt(this, "Add Event", "Day (1-31):", QDate::currentDate().day(), 1, 31, 1, &ok);
    if (!ok) return;

    CalendarEvent ev;
    ev.id = events_.size() + 1;
    ev.title = title;
    ev.category = cat;
    ev.date = QDate(displayYear_, displayMonth_, qMin(day, QDate(displayYear_, displayMonth_, 1).daysInMonth()));

    QColor catColors[] = {QColor(59,130,246), QColor(239,68,68), QColor(139,92,246), QColor(245,158,11), QColor(16,185,129)};
    int catIdx = cats.indexOf(cat);
    ev.color = catColors[qBound(0, catIdx, 4)];
    addEvent(ev);
}

void ResearchCalendarWidget::onMonthChanged(int index) {
    displayMonth_ = index + 1;
    update();
}

void ResearchCalendarWidget::onClear() {
    events_.clear();
    saveSettings();
    infoLabel_->setText("Add research events to calendar");
    update();
}

void ResearchCalendarWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (events_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add research events to calendar");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Research Calendar");

    int w = width(), h = height();
    drawCalendar(p, QRect(20, 50, w - 40, h / 2));
    drawUpcomingList(p, QRect(20, h / 2 + 20, w / 2 - 30, h / 2 - 50));
    drawCategoryChart(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void ResearchCalendarWidget::drawCalendar(QPainter& p, const QRect& rect) {
    QStringList dayNames = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

    QDate firstDay(displayYear_, displayMonth_, 1);
    int daysInMonth = firstDay.daysInMonth();
    int startDow = firstDay.dayOfWeek();

    int cellW = (rect.width() - 10) / 7;
    int cellH = qMin(50, (rect.height() - 25) / 7);

    // Headers
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8, QFont::Bold));
    for (int d = 0; d < 7; ++d) {
        p.drawText(rect.x() + 5 + d * cellW, rect.y() + 12, cellW, 14, Qt::AlignCenter, dayNames[d]);
    }

    QDate today = QDate::currentDate();

    for (int day = 1; day <= daysInMonth; ++day) {
        int idx = startDow - 1 + day - 1;
        int col = idx % 7;
        int row = idx / 7;
        int x = rect.x() + 5 + col * cellW;
        int y = rect.y() + 22 + row * cellH;

        QDate d(displayYear_, displayMonth_, day);
        bool isToday = (d == today);

        if (isToday) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(59, 130, 246, 30));
            p.drawRoundedRect(x, y, cellW - 2, cellH - 2, 4, 4);
        }

        p.setPen(isToday ? QColor(59,130,246) : QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, isToday ? QFont::Bold : QFont::Normal));
        p.drawText(x + 3, y + 2, 20, 14, Qt::AlignVCenter, QString::number(day));

        auto dayEvents = eventsForDate(d);
        for (int e = 0; e < qMin(3, dayEvents.size()); ++e) {
            p.setPen(Qt::NoPen);
            p.setBrush(dayEvents[e].color);
            p.drawRoundedRect(x + 2, y + 16 + e * 10, cellW - 6, 8, 2, 2);

            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 6));
            p.drawText(x + 4, y + 16 + e * 10, cellW - 10, 8, Qt::AlignVCenter,
                       dayEvents[e].title.left(6));
        }
    }
}

void ResearchCalendarWidget::drawUpcomingList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Upcoming");

    QDate today = QDate::currentDate();
    QList<CalendarEvent> upcoming;
    for (const auto& e : events_) {
        if (e.date >= today) upcoming.append(e);
    }
    std::sort(upcoming.begin(), upcoming.end(),
        [](const CalendarEvent& a, const CalendarEvent& b) { return a.date < b.date; });

    int itemH = qMin(24, (rect.height() - 25) / qMax(1, qMin(8, upcoming.size())));
    for (int i = 0; i < qMin(8, upcoming.size()); ++i) {
        const auto& ev = upcoming[i];
        int y = rect.y() + 22 + i * itemH;

        p.setPen(Qt::NoPen);
        p.setBrush(ev.color);
        p.drawEllipse(rect.x() + 5, y + itemH / 2 - 3, 6, 6);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 16, y, rect.width() - 60, itemH, Qt::AlignVCenter, ev.title.left(16));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 55, y, 55, itemH, Qt::AlignVCenter | Qt::AlignRight,
                   ev.date.toString("MM/dd"));
    }
}

void ResearchCalendarWidget::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = {"reading", "deadline", "meeting", "review", "submission"};
    QString labels[] = {"Reading", "Deadline", "Meeting", "Review", "Submission"};
    QColor colors[] = {QColor(59,130,246), QColor(239,68,68), QColor(139,92,246), QColor(245,158,11), QColor(16,185,129)};

    int maxVal = 1;
    for (const auto& c : counts) maxVal = qMax(maxVal, c);

    int barH = qMin(18, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void ResearchCalendarWidget::updateInfo() {
    if (events_.isEmpty()) { infoLabel_->setText("Add research events to calendar"); return; }
    QDate today = QDate::currentDate();
    int upcoming = 0;
    for (const auto& e : events_) if (e.date >= today) upcoming++;
    infoLabel_->setText(QString("%1 events | %2 upcoming | %3 categories")
        .arg(events_.size()).arg(upcoming).arg(categoryCounts().size()));
}

void ResearchCalendarWidget::loadSettings() {
    int size = settings_.beginReadArray("events");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CalendarEvent ev;
        ev.id = settings_.value("id").toInt();
        ev.title = settings_.value("title").toString();
        ev.category = settings_.value("category").toString();
        ev.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        ev.color = QColor(settings_.value("color").toString());
        ev.notes = settings_.value("notes").toString();
        events_.append(ev);
    }
    settings_.endArray();
    updateInfo();
}

void ResearchCalendarWidget::saveSettings() {
    settings_.beginWriteArray("events");
    for (int i = 0; i < events_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", events_[i].id);
        settings_.setValue("title", events_[i].title);
        settings_.setValue("category", events_[i].category);
        settings_.setValue("date", events_[i].date.toString(Qt::ISODate));
        settings_.setValue("color", events_[i].color.name());
        settings_.setValue("notes", events_[i].notes);
    }
    settings_.endArray();
}
