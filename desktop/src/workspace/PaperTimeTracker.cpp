#include "workspace/PaperTimeTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTimeTracker::PaperTimeTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TimeTracker")
{
    setupUI();
    loadSettings();
}

void PaperTimeTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperTimeTracker::onTrack);
    toolbar->addWidget(trackBtn_);
    toolbar->addWidget(new QLabel("Project:"));
    projectCombo_ = new QComboBox();
    projectCombo_->addItems({"All", "Research", "Writing", "Review", "Admin"});
    toolbar->addWidget(projectCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTimeTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter task name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Track time entries");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperTimeTracker::addEntry(const TimeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit timeTracked(entry.id, entry.hours);
    update();
}

QList<TimeEntry> PaperTimeTracker::entries() const { return entries_; }

qreal PaperTimeTracker::totalHours() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.hours;
    return t;
}

qreal PaperTimeTracker::totalBillable() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.billable;
    return t;
}

QMap<QString, int> PaperTimeTracker::projectCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.project]++;
    return counts;
}

void PaperTimeTracker::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList projects = {"research", "writing", "review", "admin"};
    QStringList categories = {"deep work", "meeting", "email", "planning"};
    int pIdx = projectCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        TimeEntry e;
        e.id = entries_.size() + 1;
        e.taskName = text.left(10) + " task" + QString::number(i);
        e.project = pIdx == 0 ? projects[QRandomGenerator::global()->bounded(projects.size())] : projects[pIdx - 1];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.hours = 0.5 + QRandomGenerator::global()->bounded(80) / 10.0;
        e.billable = e.project != "admin" ? e.hours * (0.7 + QRandomGenerator::global()->bounded(30) / 100.0) : 0;
        e.date = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(28));
        e.overtime = e.hours > 8.0;
        e.color = e.overtime ? QColor(239,68,68) : (e.billable > 0 ? QColor(16,185,129) : QColor(156,163,175));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTimeTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track time entries");
    update();
}

void PaperTimeTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track time entries");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Time Tracker");
    int w = width(), h = height();
    drawTimeList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawProjectChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTimeTracker::drawTimeList(QPainter& p, const QRect& rect) {
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
                   e.taskName.left(14) + (e.overtime ? " [OT]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.project + " | " + e.category + " | " + e.date);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.hours, 'f', 1) + "h");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.billable, 'f', 0) + " billable");
    }
}

void PaperTimeTracker::drawProjectChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Projects");
    auto counts = projectCounts();
    QStringList projects = {"research", "writing", "review", "admin"};
    QString labels[] = {"Research", "Writing", "Review", "Admin"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(projects[i]) ? counts[projects[i]] : 0;
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

void PaperTimeTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Total Hours", QString::number(totalHours(), 'f', 1), QColor(16,185,129)},
        {"Billable", "$" + QString::number(totalBillable(), 'f', 0), QColor(245,158,11)},
        {"Projects", QString::number(projectCounts().size()), QColor(139,92,246)}
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

void PaperTimeTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track time entries"); return; }
    infoLabel_->setText(QString("%1 entries | %2h total | $%3 bill")
        .arg(entries_.size()).arg(totalHours(), 0, 'f', 1).arg(totalBillable(), 0, 'f', 0));
}

void PaperTimeTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TimeEntry e;
        e.id = settings_.value("id").toInt();
        e.taskName = settings_.value("taskName").toString();
        e.project = settings_.value("project").toString();
        e.category = settings_.value("category").toString();
        e.hours = settings_.value("hours").toDouble();
        e.billable = settings_.value("billable").toDouble();
        e.date = settings_.value("date").toString();
        e.overtime = settings_.value("overtime").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTimeTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("taskName", entries_[i].taskName);
        settings_.setValue("project", entries_[i].project);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("hours", entries_[i].hours);
        settings_.setValue("billable", entries_[i].billable);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("overtime", entries_[i].overtime);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
