#include "visualization/LiteratureTimelineWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

LiteratureTimelineWidget::LiteratureTimelineWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LitTimeline")
{
    setupUI();
    loadSettings();
}

void LiteratureTimelineWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Zoom:"));
    zoomCombo_ = new QComboBox();
    zoomCombo_->addItems({"Year", "Quarter", "Month", "All"});
    connect(zoomCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LiteratureTimelineWidget::onZoomChanged);
    toolbar->addWidget(zoomCombo_, 1);

    addBtn_ = new QPushButton("Add Event");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &LiteratureTimelineWidget::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &LiteratureTimelineWidget::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add events to literature timeline");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 450);
}

void LiteratureTimelineWidget::addEvent(const TimelineEvent& event) {
    events_.append(event);
    saveSettings();
    updateInfo();
    emit timelineUpdated(events_.size());
    update();
}

QList<TimelineEvent> LiteratureTimelineWidget::events() const { return events_; }

QMap<QString, int> LiteratureTimelineWidget::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : events_) counts[e.category]++;
    return counts;
}

QDate LiteratureTimelineWidget::earliestDate() const {
    if (events_.isEmpty()) return QDate::currentDate();
    QDate d = events_.first().date;
    for (const auto& e : events_) if (e.date < d) d = e.date;
    return d;
}

QDate LiteratureTimelineWidget::latestDate() const {
    if (events_.isEmpty()) return QDate::currentDate();
    QDate d = events_.first().date;
    for (const auto& e : events_) if (e.date > d) d = e.date;
    return d;
}

void LiteratureTimelineWidget::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Event", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QStringList cats = {"publication", "milestone", "conference", "breakthrough"};
    QString cat = QInputDialog::getItem(this, "Add Event", "Category:", cats, 0, false, &ok);
    if (!ok) return;
    int impact = QInputDialog::getInt(this, "Add Event", "Impact (1-10):", 5, 1, 10, 1, &ok);
    if (!ok) return;
    QString desc = QInputDialog::getText(this, "Add Event", "Description:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    TimelineEvent ev;
    ev.id = events_.size() + 1;
    ev.title = title;
    ev.description = desc;
    ev.category = cat;
    ev.date = QDate::currentDate();
    ev.impact = impact;

    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};
    int catIdx = cats.indexOf(cat);
    ev.color = catColors[qBound(0, catIdx, 3)];
    addEvent(ev);
}

void LiteratureTimelineWidget::onZoomChanged(int) { update(); }
void LiteratureTimelineWidget::onClear() {
    events_.clear();
    selectedEvent_ = -1;
    saveSettings();
    infoLabel_->setText("Add events to literature timeline");
    update();
}

void LiteratureTimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (events_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add events to literature timeline");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Literature Timeline");

    int w = width(), h = height();
    drawTimeline(p, QRect(20, 50, w - 40, h / 2));
    drawCategoryLegend(p, QRect(20, h / 2 + 20, w / 2 - 30, h / 2 - 50));
    drawImpactChart(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void LiteratureTimelineWidget::drawTimeline(QPainter& p, const QRect& rect) {
    if (events_.isEmpty()) return;

    QList<TimelineEvent> sorted = events_;
    std::sort(sorted.begin(), sorted.end(),
        [](const TimelineEvent& a, const TimelineEvent& b) { return a.date < b.date; });

    QDate minD = sorted.first().date;
    QDate maxD = sorted.last().date;
    int totalDays = qMax(1, minD.daysTo(maxD));

    int lineY = rect.y() + rect.height() / 2;

    p.setPen(QPen(QColor(203, 213, 225), 2));
    p.drawLine(rect.x() + 10, lineY, rect.right() - 10, lineY);

    for (int i = 0; i < sorted.size(); ++i) {
        const auto& ev = sorted[i];
        qreal x = rect.x() + 10 + (static_cast<qreal>(minD.daysTo(ev.date)) / totalDays) * (rect.width() - 20);
        bool above = (i % 2 == 0);
        int dotY = lineY;

        p.setPen(Qt::NoPen);
        p.setBrush(ev.color);
        p.drawEllipse(QPointF(x, dotY), 6, 6);

        int textY = above ? dotY - 30 : dotY + 10;

        p.setPen(QPen(ev.color, 1, Qt::DashLine));
        p.drawLine(static_cast<int>(x), dotY + (above ? -6 : 6), static_cast<int>(x), textY + (above ? 26 : -4));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(static_cast<int>(x) - 45, textY, 90, 14, Qt::AlignCenter, ev.title.left(12));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(static_cast<int>(x) - 45, textY + 12, 90, 12, Qt::AlignCenter, ev.date.toString("yyyy-MM"));
    }

    QDate today = QDate::currentDate();
    if (today >= minD && today <= maxD) {
        qreal tx = rect.x() + 10 + (static_cast<qreal>(minD.daysTo(today)) / totalDays) * (rect.width() - 20);
        p.setPen(QPen(QColor(239, 68, 68), 1, Qt::DashLine));
        p.drawLine(static_cast<int>(tx), rect.y(), static_cast<int>(tx), rect.bottom());
        p.setPen(QColor(239, 68, 68));
        p.setFont(QFont("Arial", 7));
        p.drawText(static_cast<int>(tx) + 3, rect.y() + 10, "today");
    }
}

void LiteratureTimelineWidget::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"publication", "milestone", "conference", "breakthrough"};
    QString labels[] = {"Publications", "Milestones", "Conferences", "Breakthroughs"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int itemH = qMin(30, (rect.height() - 25) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * itemH;
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 2, 12, 12, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 22, y, rect.width() - 50, 16, Qt::AlignVCenter, labels[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 30, y, 30, 16, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count));
    }
}

void LiteratureTimelineWidget::drawImpactChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Impact Over Time");

    QList<TimelineEvent> sorted = events_;
    std::sort(sorted.begin(), sorted.end(),
        [](const TimelineEvent& a, const TimelineEvent& b) { return a.date < b.date; });

    int show = qMin(10, sorted.size());
    int barW = qMin(30, (rect.width() - 20) / show);

    for (int i = 0; i < show; ++i) {
        const auto& ev = sorted[sorted.size() - show + i];
        int x = rect.x() + 10 + i * barW;
        qreal h = (static_cast<qreal>(ev.impact) / 10) * (rect.height() - 50);

        p.setPen(Qt::NoPen);
        p.setBrush(ev.color);
        p.drawRoundedRect(x, rect.bottom() - 25 - static_cast<int>(h), barW - 3, static_cast<int>(h), 2, 2);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(x - 2, rect.bottom() - 8, barW, 12, Qt::AlignCenter, ev.date.toString("MM/yy"));
    }
}

void LiteratureTimelineWidget::updateInfo() {
    if (events_.isEmpty()) { infoLabel_->setText("Add events to literature timeline"); return; }
    infoLabel_->setText(QString("%1 events | %2 categories | %3 to %4")
        .arg(events_.size()).arg(categoryCounts().size())
        .arg(earliestDate().toString("yyyy"))
        .arg(latestDate().toString("yyyy")));
}

void LiteratureTimelineWidget::loadSettings() {
    int size = settings_.beginReadArray("events");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TimelineEvent ev;
        ev.id = settings_.value("id").toInt();
        ev.title = settings_.value("title").toString();
        ev.description = settings_.value("description").toString();
        ev.category = settings_.value("category").toString();
        ev.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        ev.impact = settings_.value("impact").toInt();
        ev.color = QColor(settings_.value("color").toString());
        events_.append(ev);
    }
    settings_.endArray();
    updateInfo();
}

void LiteratureTimelineWidget::saveSettings() {
    settings_.beginWriteArray("events");
    for (int i = 0; i < events_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", events_[i].id);
        settings_.setValue("title", events_[i].title);
        settings_.setValue("description", events_[i].description);
        settings_.setValue("category", events_[i].category);
        settings_.setValue("date", events_[i].date.toString(Qt::ISODate));
        settings_.setValue("impact", events_[i].impact);
        settings_.setValue("color", events_[i].color.name());
    }
    settings_.endArray();
}
