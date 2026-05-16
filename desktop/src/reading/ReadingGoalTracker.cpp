#include "reading/ReadingGoalTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QDateEdit>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

ReadingGoalTracker::ReadingGoalTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingGoals")
{
    setupUI();
    loadSettings();
}

void ReadingGoalTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"Active", "Completed", "All", "Overdue"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingGoalTracker::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    addBtn_ = new QPushButton("Add Goal");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &ReadingGoalTracker::onAdd);
    toolbar->addWidget(addBtn_);

    logBtn_ = new QPushButton("Log Progress");
    logBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(logBtn_, &QPushButton::clicked, this, &ReadingGoalTracker::onLogProgress);
    toolbar->addWidget(logBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &ReadingGoalTracker::onDelete);
    toolbar->addWidget(deleteBtn_);
    layout->addLayout(toolbar);

    setMinimumSize(500, 400);
}

void ReadingGoalTracker::addGoal(const Goal& goal) {
    Goal g = goal;
    if (g.id < 0) g.id = nextId_++;
    goals_.append(g);
    nextId_ = qMax(nextId_, g.id + 1);
    saveSettings();
    updateStats();
    update();
    emit goalCreated(g.id);
}

void ReadingGoalTracker::removeGoal(int goalId) {
    goals_.removeIf([goalId](const Goal& g) { return g.id == goalId; });
    if (selectedId_ == goalId) selectedId_ = -1;
    saveSettings();
    updateStats();
    update();
    emit goalRemoved(goalId);
}

void ReadingGoalTracker::updateProgress(int goalId, int value) {
    for (auto& g : goals_) {
        if (g.id == goalId) {
            g.current = qMin(value, g.target);
            qreal pct = progress(goalId);
            emit goalProgressChanged(goalId, pct);
            if (g.current >= g.target) emit goalCompleted(goalId);
            break;
        }
    }
    saveSettings();
    updateStats();
    update();
}

QList<Goal> ReadingGoalTracker::goals() const { return goals_; }

QList<Goal> ReadingGoalTracker::activeGoals() const {
    QList<Goal> result;
    for (const auto& g : goals_) {
        if (g.active && g.current < g.target) result.append(g);
    }
    return result;
}

qreal ReadingGoalTracker::progress(int goalId) const {
    for (const auto& g : goals_) {
        if (g.id == goalId && g.target > 0) {
            return qMin(1.0, static_cast<qreal>(g.current) / g.target);
        }
    }
    return 0;
}

void ReadingGoalTracker::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "New Goal", "Goal name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    int target = QInputDialog::getInt(this, "New Goal", "Target (papers/pages/minutes):", 20, 1, 1000, 1, &ok);
    if (!ok) return;

    Goal g;
    g.name = name.trimmed();
    g.target = target;
    g.startDate = QDate::currentDate();
    g.deadline = QDate::currentDate().addMonths(1);
    g.active = true;
    addGoal(g);
}

void ReadingGoalTracker::onDelete() {
    if (selectedId_ < 0) {
        bool ok;
        int id = QInputDialog::getInt(this, "Delete Goal", "Goal ID:", 1, 1, 1000, 1, &ok);
        if (ok) removeGoal(id);
    } else {
        removeGoal(selectedId_);
    }
}

void ReadingGoalTracker::onLogProgress() {
    bool ok;
    int id = QInputDialog::getInt(this, "Log Progress", "Goal ID:", selectedId_ > 0 ? selectedId_ : 1, 1, 1000, 1, &ok);
    if (!ok) return;
    int current = 0;
    for (const auto& g : goals_) {
        if (g.id == id) { current = g.current; break; }
    }
    int value = QInputDialog::getInt(this, "Log Progress", "New progress:", current + 1, 0, 10000, 1, &ok);
    if (ok) updateProgress(id, value);
}

void ReadingGoalTracker::onFilterChanged(int) { update(); }

void ReadingGoalTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (goals_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No goals. Click 'Add Goal' to start.");
        return;
    }

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(20, 30, "Reading Goals");

    drawProgressRings(p, QRect(20, 50, width() - 40, qMin(160, 50 + goals_.size() * 40)));
    drawGoalCards(p, QRect(20, 220, width() - 40, height() - 260));
}

void ReadingGoalTracker::drawProgressRings(QPainter& p, const QRect& rect) {
    int n = qMin(goals_.size(), 6);
    int ringSize = qMin(60, rect.width() / (n * 2));
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(14,165,233)};

    for (int i = 0; i < n; ++i) {
        const auto& g = goals_[i];
        qreal cx = rect.x() + ringSize + i * (ringSize * 2 + 10);
        qreal cy = rect.y() + ringSize + 10;
        qreal pct = (g.target > 0) ? static_cast<qreal>(g.current) / g.target : 0;

        // Background ring
        p.setPen(QPen(QColor(226, 232, 240), 5));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(cx, cy), ringSize - 5, ringSize - 5);

        // Progress arc
        p.setPen(QPen(colors[i % 6], 5, Qt::SolidLine, Qt::RoundCap));
        int spanAngle = static_cast<int>(pct * 360 * 16);
        p.drawArc(static_cast<int>(cx - ringSize + 5), static_cast<int>(cy - ringSize + 5),
                  (ringSize - 5) * 2, (ringSize - 5) * 2, 90 * 16, -spanAngle);

        // Center text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(QRectF(cx - ringSize + 5, cy - 8, (ringSize - 5) * 2, 16),
                   Qt::AlignCenter, QString::number(static_cast<int>(pct * 100)) + "%");

        // Label below
        p.setFont(QFont("Arial", 7));
        p.drawText(QRectF(cx - ringSize, cy + ringSize - 8, ringSize * 2, 14),
                   Qt::AlignCenter, g.name.left(12));
    }
}

void ReadingGoalTracker::drawGoalCards(QPainter& p, const QRect& rect) {
    int filter = filterCombo_->currentIndex();
    int cardH = qMin(35, (rect.height() - 10) / qMax(1, goals_.size()));
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(14,165,233)};

    int y = rect.y();
    for (int i = 0; i < goals_.size(); ++i) {
        const auto& g = goals_[i];
        qreal pct = (g.target > 0) ? static_cast<qreal>(g.current) / g.target : 0;
        bool completed = g.current >= g.target;
        bool overdue = !completed && g.deadline < QDate::currentDate();

        if (filter == 0 && (completed || !g.active)) continue;
        if (filter == 1 && !completed) continue;
        if (filter == 3 && !overdue) continue;

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH - 3, 4, 4);

        // Progress bar
        int barW = rect.width() - 200;
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x() + 140, y + 8, barW, cardH - 20, 3, 3);

        QColor barColor = completed ? QColor(16, 185, 129) : overdue ? QColor(239, 68, 68) : colors[i % 6];
        p.setBrush(barColor);
        p.drawRoundedRect(rect.x() + 140, y + 8, static_cast<int>(barW * pct), cardH - 20, 3, 3);

        // Text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 5, y + cardH - 10, g.name.left(20));

        p.setFont(QFont("Arial", 8));
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 145 + barW + 5, y + cardH - 10,
                   QString("%1/%2").arg(g.current).arg(g.target));

        if (!g.deadline.isNull()) {
            p.drawText(rect.x() + rect.width() - 70, y + cardH - 10, g.deadline.toString("MM-dd"));
        }

        y += cardH;
        if (y > rect.bottom()) break;
    }
}

void ReadingGoalTracker::refreshList() { update(); }

void ReadingGoalTracker::updateStats() {
    int active = 0, completed = 0;
    for (const auto& g : goals_) {
        if (g.current >= g.target) completed++;
        else if (g.active) active++;
    }
    statsLabel_->setText(QString("%1 goals (%2 active, %3 done)").arg(goals_.size()).arg(active).arg(completed));
}

void ReadingGoalTracker::loadSettings() {
    QSettings s("PaperCrawler", "ReadingGoals");
    QByteArray data = s.value("goals").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        Goal g;
        g.id = obj["id"].toInt();
        g.name = obj["name"].toString();
        g.type = obj["type"].toString("papers");
        g.target = obj["target"].toInt();
        g.current = obj["current"].toInt();
        g.deadline = QDate::fromString(obj["deadline"].toString(), Qt::ISODate);
        g.startDate = QDate::fromString(obj["startDate"].toString(), Qt::ISODate);
        g.active = obj["active"].toBool(true);
        goals_.append(g);
        nextId_ = qMax(nextId_, g.id + 1);
    }
    updateStats();
}

void ReadingGoalTracker::saveSettings() {
    QJsonArray arr;
    for (const auto& g : goals_) {
        QJsonObject obj;
        obj["id"] = g.id;
        obj["name"] = g.name;
        obj["type"] = g.type;
        obj["target"] = g.target;
        obj["current"] = g.current;
        obj["deadline"] = g.deadline.toString(Qt::ISODate);
        obj["startDate"] = g.startDate.toString(Qt::ISODate);
        obj["active"] = g.active;
        arr.append(obj);
    }
    QSettings s("PaperCrawler", "ReadingGoals");
    s.setValue("goals", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
