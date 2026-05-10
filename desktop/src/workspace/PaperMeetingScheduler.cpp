#include "workspace/PaperMeetingScheduler.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMeetingScheduler::PaperMeetingScheduler(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MeetingScheduler")
{
    setupUI();
    loadSettings();
}

void PaperMeetingScheduler::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    scheduleBtn_ = new QPushButton("Schedule");
    scheduleBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scheduleBtn_, &QPushButton::clicked, this, &PaperMeetingScheduler::onSchedule);
    toolbar->addWidget(scheduleBtn_);
    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"All", "Standup", "Review", "Planning", "Social"});
    toolbar->addWidget(typeCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMeetingScheduler::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter meeting title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Schedule meetings");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperMeetingScheduler::addEntry(const MeetingEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit meetingScheduled(entry.id, entry.duration);
    update();
}

QList<MeetingEntry> PaperMeetingScheduler::entries() const { return entries_; }

int PaperMeetingScheduler::totalMinutes() const {
    int t = 0;
    for (const auto& e : entries_) t += e.duration;
    return t;
}

int PaperMeetingScheduler::recurringCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.recurring) c++;
    return c;
}

QMap<QString, int> PaperMeetingScheduler::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.type]++;
    return counts;
}

void PaperMeetingScheduler::onSchedule() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList types = {"standup", "review", "planning", "social"};
    QStringList statuses = {"confirmed", "tentative", "pending"};
    QStringList organizers = {"alice", "bob", "carol", "dave"};
    int tIdx = typeCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        MeetingEntry e;
        e.id = entries_.size() + 1;
        e.title = text.left(10) + " mtg" + QString::number(i);
        e.organizer = organizers[QRandomGenerator::global()->bounded(organizers.size())];
        e.date = "2026-05-" + QString::number(10 + QRandomGenerator::global()->bounded(20));
        e.duration = 15 + QRandomGenerator::global()->bounded(4) * 15;
        e.attendees = 2 + QRandomGenerator::global()->bounded(15);
        e.type = tIdx == 0 ? types[QRandomGenerator::global()->bounded(types.size())] : types[tIdx - 1];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.recurring = QRandomGenerator::global()->bounded(3) == 0;
        e.color = e.status == "confirmed" ? QColor(16,185,129) : (e.status == "tentative" ? QColor(245,158,11) : QColor(156,163,175));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperMeetingScheduler::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Schedule meetings");
    update();
}

void PaperMeetingScheduler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Schedule meetings");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Meeting Scheduler");
    int w = width(), h = height();
    drawMeetingList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperMeetingScheduler::drawMeetingList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.title.left(14) + (e.recurring ? " [R]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.organizer + " | " + QString::number(e.attendees) + " ppl | " + e.date);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.duration) + "min");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.type + " | " + e.status);
    }
}

void PaperMeetingScheduler::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Types");
    auto counts = typeCounts();
    QStringList types = {"standup", "review", "planning", "social"};
    QString labels[] = {"Standup", "Review", "Planning", "Social"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperMeetingScheduler::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Meetings", QString::number(entries_.size()), QColor(59,130,246)},
        {"Total Time", QString::number(totalMinutes()) + "min", QColor(16,185,129)},
        {"Recurring", QString::number(recurringCount()), QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperMeetingScheduler::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Schedule meetings"); return; }
    infoLabel_->setText(QString("%1 meetings | %2min total | %3 recurring")
        .arg(entries_.size()).arg(totalMinutes()).arg(recurringCount()));
}

void PaperMeetingScheduler::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MeetingEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.organizer = settings_.value("organizer").toString();
        e.date = settings_.value("date").toString();
        e.duration = settings_.value("duration").toInt();
        e.attendees = settings_.value("attendees").toInt();
        e.type = settings_.value("type").toString();
        e.status = settings_.value("status").toString();
        e.recurring = settings_.value("recurring").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperMeetingScheduler::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("organizer", entries_[i].organizer);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("attendees", entries_[i].attendees);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("recurring", entries_[i].recurring);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
