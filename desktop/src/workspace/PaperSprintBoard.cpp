#include "workspace/PaperSprintBoard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSprintBoard::PaperSprintBoard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SprintBoard")
{
    setupUI();
    loadSettings();
}

void PaperSprintBoard::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperSprintBoard::onAdd);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(new QLabel("Sprint:"));
    sprintCombo_ = new QComboBox();
    sprintCombo_->addItems({"All", "Sprint 1", "Sprint 2", "Sprint 3"});
    toolbar->addWidget(sprintCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSprintBoard::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter task name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Sprint board");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperSprintBoard::addEntry(const SprintTask& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sprintUpdated(entry.id, entry.progress);
    update();
}

QList<SprintTask> PaperSprintBoard::entries() const { return entries_; }

int PaperSprintBoard::completedPoints() const {
    int pts = 0;
    for (const auto& e : entries_) if (e.status == "done") pts += e.storyPoints;
    return pts;
}

int PaperSprintBoard::blockedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.blocked) c++;
    return c;
}

QMap<QString, int> PaperSprintBoard::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.status]++;
    return counts;
}

void PaperSprintBoard::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList statuses = {"todo", "progress", "review", "done"};
    QStringList priorities = {"critical", "high", "medium", "low"};
    QStringList sprints = {"Sprint 1", "Sprint 2", "Sprint 3"};
    QStringList assignees = {"alice", "bob", "carol", "dave"};
    int sIdx = sprintCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        SprintTask e;
        e.id = entries_.size() + 1;
        e.taskName = text.left(10) + " task" + QString::number(i);
        e.assignee = assignees[QRandomGenerator::global()->bounded(assignees.size())];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.priority = priorities[QRandomGenerator::global()->bounded(priorities.size())];
        e.storyPoints = 1 + QRandomGenerator::global()->bounded(13);
        e.progress = e.status == "done" ? 100 : QRandomGenerator::global()->bounded(90);
        e.sprint = sIdx == 0 ? sprints[QRandomGenerator::global()->bounded(sprints.size())] : sprints[sIdx - 1];
        e.blocked = e.status == "progress" && QRandomGenerator::global()->bounded(5) == 0;
        e.color = e.status == "done" ? QColor(16,185,129) :
                  e.status == "progress" ? QColor(59,130,246) :
                  e.status == "review" ? QColor(245,158,11) : QColor(156,163,175);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperSprintBoard::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Sprint board");
    update();
}

void PaperSprintBoard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Sprint board");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Sprint Board");
    int w = width(), h = height();
    drawSprintView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSprintBoard::drawSprintView(QPainter& p, const QRect& rect) {
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
                   e.taskName.left(14) + (e.blocked ? " [BLK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.assignee + " | " + e.priority + " | " + QString::number(e.storyPoints) + "pts");
        int barW = static_cast<int>((e.progress / 100.0) * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22,
                   QString::number(e.progress) + "% " + e.sprint);
    }
}

void PaperSprintBoard::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status");
    auto counts = statusCounts();
    QStringList statuses = {"todo", "progress", "review", "done"};
    QString labels[] = {"To Do", "In Progress", "Review", "Done"};
    QColor colors[] = {QColor(156,163,175), QColor(59,130,246), QColor(245,158,11), QColor(16,185,129)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
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

void PaperSprintBoard::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tasks", QString::number(entries_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completedPoints()) + "pts", QColor(16,185,129)},
        {"Blocked", QString::number(blockedCount()), QColor(239,68,68)},
        {"Statuses", QString::number(statusCounts().size()), QColor(139,92,246)}
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

void PaperSprintBoard::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Sprint board"); return; }
    infoLabel_->setText(QString("%1 tasks | %2pts done | %3 blocked")
        .arg(entries_.size()).arg(completedPoints()).arg(blockedCount()));
}

void PaperSprintBoard::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SprintTask e;
        e.id = settings_.value("id").toInt();
        e.taskName = settings_.value("taskName").toString();
        e.assignee = settings_.value("assignee").toString();
        e.status = settings_.value("status").toString();
        e.priority = settings_.value("priority").toString();
        e.storyPoints = settings_.value("storyPoints").toInt();
        e.progress = settings_.value("progress").toInt();
        e.sprint = settings_.value("sprint").toString();
        e.blocked = settings_.value("blocked").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSprintBoard::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("taskName", entries_[i].taskName);
        settings_.setValue("assignee", entries_[i].assignee);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("priority", entries_[i].priority);
        settings_.setValue("storyPoints", entries_[i].storyPoints);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("sprint", entries_[i].sprint);
        settings_.setValue("blocked", entries_[i].blocked);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
