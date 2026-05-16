#include "visualization/PaperResearchTimeline.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperResearchTimeline::PaperResearchTimeline(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ResearchTimeline")
{
    setupUI();
    loadSettings();
}

void PaperResearchTimeline::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Event");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperResearchTimeline::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperResearchTimeline::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track research events");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperResearchTimeline::addEvent(const ResearchEvent& event) {
    events_.append(event);
    saveSettings();
    updateInfo();
    emit timelineUpdated(events_.size());
    update();
}

QList<ResearchEvent> PaperResearchTimeline::events() const { return events_; }

QMap<QString, int> PaperResearchTimeline::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : events_) counts[e.eventType]++;
    return counts;
}

int PaperResearchTimeline::eventsThisMonth() const {
    int c = 0;
    QDate now = QDate::currentDate();
    for (const auto& e : events_) {
        if (e.date.year() == now.year() && e.date.month() == now.month()) c++;
    }
    return c;
}

void PaperResearchTimeline::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Event", "Event title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QStringList types = {"publication", "review", "conference", "milestone"};
    QString type = QInputDialog::getItem(this, "Add Event", "Type:", types, 0, false, &ok);
    if (!ok) return;

    ResearchEvent e;
    e.id = events_.size() + 1;
    e.title = title;
    e.eventType = type;
    e.date = QDate::currentDate().addDays(-QRandomGenerator::global()->bounded(365));
    e.description = title;

    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int tIdx = types.indexOf(type);
    e.color = typeColors[qBound(0, tIdx, 3)];
    addEvent(e);
}

void PaperResearchTimeline::onClear() {
    events_.clear();
    selectedEvent_ = -1;
    saveSettings();
    infoLabel_->setText("Track research events");
    update();
}

void PaperResearchTimeline::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (events_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track research events");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Research Timeline");

    int w = width(), h = height();
    drawTimelineTrack(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperResearchTimeline::drawTimelineTrack(QPainter& p, const QRect& rect) {
    int trackY = rect.y() + rect.height() / 2;
    p.setPen(QPen(QColor(203, 213, 225), 2));
    p.drawLine(rect.x(), trackY, rect.x() + rect.width(), trackY);

    int show = qMin(12, events_.size());
    int spacing = qMin(38, (rect.width() - 20) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = events_[events_.size() - 1 - i];
        int x = rect.x() + rect.width() - 10 - i * spacing;
        bool above = (i % 2 == 0);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(x - 5, trackY - 5, 10, 10);

        p.setPen(QPen(QColor(203, 213, 225), 1));
        int textY = above ? trackY - 30 : trackY + 10;
        p.drawLine(x, trackY, x, above ? trackY - 15 : trackY + 15);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(x - 30, textY, 60, 14, Qt::AlignCenter, e.title.left(10));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(x - 30, textY + 12, 60, 12, Qt::AlignCenter, e.date.toString("MM/dd"));
    }
}

void PaperResearchTimeline::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Event Type");

    auto counts = typeCounts();
    QStringList types = {"publication", "review", "conference", "milestone"};
    QString labels[] = {"Publication", "Review", "Conference", "Milestone"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int total = events_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 20 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        qreal span = total > 0 ? (static_cast<qreal>(count) / total) * 360 : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperResearchTimeline::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Events", QString::number(events_.size()), QColor(59,130,246)},
        {"This Month", QString::number(eventsThisMonth()), QColor(16,185,129)},
        {"Types", QString::number(typeCounts().size()), QColor(245,158,11)},
        {"Publications", QString::number(typeCounts().value("publication", 0)), QColor(139,92,246)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperResearchTimeline::updateInfo() {
    if (events_.isEmpty()) { infoLabel_->setText("Track research events"); return; }
    infoLabel_->setText(QString("%1 events | %2 this month | %3 publications")
        .arg(events_.size()).arg(eventsThisMonth()).arg(typeCounts().value("publication", 0)));
}

void PaperResearchTimeline::loadSettings() {
    int size = settings_.beginReadArray("events");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ResearchEvent e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.eventType = settings_.value("eventType").toString();
        e.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        e.description = settings_.value("description").toString();
        e.color = QColor(settings_.value("color").toString());
        events_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperResearchTimeline::saveSettings() {
    settings_.beginWriteArray("events");
    for (int i = 0; i < events_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", events_[i].id);
        settings_.setValue("title", events_[i].title);
        settings_.setValue("eventType", events_[i].eventType);
        settings_.setValue("date", events_[i].date.toString(Qt::ISODate));
        settings_.setValue("description", events_[i].description);
        settings_.setValue("color", events_[i].color.name());
    }
    settings_.endArray();
}
