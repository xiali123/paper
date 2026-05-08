#include "PaperCollaborationBoard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperCollaborationBoard::PaperCollaborationBoard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CollabBoard")
{
    setupUI();
    loadSettings();
}

void PaperCollaborationBoard::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "To Do", "In Progress", "Review", "Done"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCollaborationBoard::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    addBtn_ = new QPushButton("Add Task");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperCollaborationBoard::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCollaborationBoard::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add tasks to collaboration board");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 450);
}

void PaperCollaborationBoard::addTask(const CollabTask& task) {
    tasks_.append(task);
    saveSettings();
    updateInfo();
    int done = 0;
    for (const auto& t : tasks_) if (t.status == "done") done++;
    emit boardUpdated(tasks_.size(), done);
    update();
}

QList<CollabTask> PaperCollaborationBoard::tasks() const { return tasks_; }

QMap<QString, int> PaperCollaborationBoard::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& t : tasks_) counts[t.status]++;
    return counts;
}

int PaperCollaborationBoard::overdueCount() const {
    int c = 0;
    QDate today = QDate::currentDate();
    for (const auto& t : tasks_) {
        if (t.status != "done" && t.dueDate.isValid() && t.dueDate < today) c++;
    }
    return c;
}

void PaperCollaborationBoard::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Task", "Task title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QString assignee = QInputDialog::getText(this, "Add Task", "Assignee:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QStringList priorities = {"low", "medium", "high", "urgent"};
    QString priority = QInputDialog::getItem(this, "Add Task", "Priority:", priorities, 1, false, &ok);
    if (!ok) return;

    CollabTask t;
    t.id = tasks_.size() + 1;
    t.title = title;
    t.assignee = assignee;
    t.status = "todo";
    t.priority = priority;
    t.dueDate = QDate::currentDate().addDays(7);
    QColor pColors[] = {QColor(16,185,129), QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};
    int pIdx = priorities.indexOf(priority);
    t.color = pColors[qBound(0, pIdx, 3)];
    addTask(t);
}

void PaperCollaborationBoard::onFilterChanged(int) { update(); }
void PaperCollaborationBoard::onClear() {
    tasks_.clear();
    selectedTask_ = -1;
    saveSettings();
    infoLabel_->setText("Add tasks to collaboration board");
    update();
}

void PaperCollaborationBoard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (tasks_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add tasks to collaboration board");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Collaboration Board");

    int w = width(), h = height();
    drawBoardColumns(p, QRect(20, 50, w - 40, h * 2 / 3 - 40));
    drawStats(p, QRect(20, h * 2 / 3 + 10, w - 40, h / 3 - 30));
}

void PaperCollaborationBoard::drawBoardColumns(QPainter& p, const QRect& rect) {
    QStringList statuses = {"todo", "in_progress", "review", "done"};
    QStringList statusLabels = {"To Do", "In Progress", "Review", "Done"};
    QColor colColors[] = {QColor(245,158,11), QColor(59,130,246), QColor(139,92,246), QColor(16,185,129)};

    int colW = rect.width() / 4;
    int filterIdx = filterCombo_->currentIndex();

    for (int c = 0; c < 4; ++c) {
        if (filterIdx > 0 && filterIdx - 1 != c) continue;

        int x = rect.x() + c * colW;
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(colColors[c].lighter(195));
        p.drawRoundedRect(x + 2, y, colW - 4, 24, 4, 4);

        p.setPen(colColors[c]);
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(x + 8, y + 16, statusLabels[c]);

        int count = 0;
        for (const auto& t : tasks_) {
            if (t.status != statuses[c]) continue;
            int cardY = y + 30 + count * 48;
            if (cardY + 44 > rect.bottom()) break;

            bool sel = (t.id == selectedTask_);
            p.setPen(Qt::NoPen);
            p.setBrush(sel ? QColor(241, 245, 249) : Qt::white);
            p.drawRoundedRect(x + 6, cardY, colW - 12, 44, 4, 4);

            p.setPen(QColor(15, 23, 42));
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(x + 12, cardY + 14, colW - 24, 14, Qt::AlignVCenter, t.title.left(18));

            p.setPen(QColor(100, 116, 139));
            p.setFont(QFont("Arial", 7));
            p.drawText(x + 12, cardY + 28, colW - 24, 14, Qt::AlignVCenter,
                       t.assignee.isEmpty() ? "Unassigned" : t.assignee.left(12));

            p.setPen(Qt::NoPen);
            p.setBrush(t.color);
            p.drawEllipse(x + colW - 20, cardY + 6, 8, 8);

            if (t.status != "done" && t.dueDate.isValid() && t.dueDate < QDate::currentDate()) {
                p.setPen(QColor(239, 68, 68));
                p.setFont(QFont("Arial", 6));
                p.drawText(x + 12, cardY + 40, "overdue");
            }

            count++;
        }
    }
}

void PaperCollaborationBoard::drawStats(QPainter& p, const QRect& rect) {
    auto counts = statusCounts();
    QStringList labels = {"todo", "in_progress", "review", "done"};
    QString displayNames[] = {"To Do", "In Progress", "Review", "Done"};
    QColor colors[] = {QColor(245,158,11), QColor(59,130,246), QColor(139,92,246), QColor(16,185,129)};

    int total = tasks_.size();
    if (total == 0) return;

    int boxW = qMin(140, rect.width() / 5);

    for (int i = 0; i < 4; ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y();
        int count = counts.contains(labels[i]) ? counts[labels[i]] : 0;
        qreal pct = static_cast<qreal>(count) / total;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i].lighter(190));
        p.drawRoundedRect(x, y, boxW, 45, 6, 6);

        p.setPen(colors[i]);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 8, y + 5, boxW - 16, 22, Qt::AlignVCenter, QString::number(count));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 8, y + 28, boxW - 16, 14, Qt::AlignVCenter,
                   displayNames[i] + " " + QString::number(static_cast<int>(pct * 100)) + "%");
    }

    int overdue = overdueCount();
    if (overdue > 0) {
        int x = rect.x() + 4 * (boxW + 10);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(239, 68, 68, 30));
        p.drawRoundedRect(x, rect.y(), boxW, 45, 6, 6);

        p.setPen(QColor(239, 68, 68));
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 8, rect.y() + 5, boxW - 16, 22, Qt::AlignVCenter, QString::number(overdue));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 8, rect.y() + 28, boxW - 16, 14, Qt::AlignVCenter, "Overdue");
    }
}

void PaperCollaborationBoard::updateInfo() {
    if (tasks_.isEmpty()) { infoLabel_->setText("Add tasks to collaboration board"); return; }
    int done = 0;
    for (const auto& t : tasks_) if (t.status == "done") done++;
    infoLabel_->setText(QString("%1 tasks | %2 done | %3 overdue")
        .arg(tasks_.size()).arg(done).arg(overdueCount()));
}

void PaperCollaborationBoard::loadSettings() {
    int size = settings_.beginReadArray("tasks");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CollabTask t;
        t.id = settings_.value("id").toInt();
        t.title = settings_.value("title").toString();
        t.assignee = settings_.value("assignee").toString();
        t.status = settings_.value("status").toString();
        t.priority = settings_.value("priority").toString();
        t.paperTitle = settings_.value("paperTitle").toString();
        t.dueDate = QDate::fromString(settings_.value("dueDate").toString(), Qt::ISODate);
        t.color = QColor(settings_.value("color").toString());
        tasks_.append(t);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCollaborationBoard::saveSettings() {
    settings_.beginWriteArray("tasks");
    for (int i = 0; i < tasks_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", tasks_[i].id);
        settings_.setValue("title", tasks_[i].title);
        settings_.setValue("assignee", tasks_[i].assignee);
        settings_.setValue("status", tasks_[i].status);
        settings_.setValue("priority", tasks_[i].priority);
        settings_.setValue("paperTitle", tasks_[i].paperTitle);
        settings_.setValue("dueDate", tasks_[i].dueDate.toString(Qt::ISODate));
        settings_.setValue("color", tasks_[i].color.name());
    }
    settings_.endArray();
}
