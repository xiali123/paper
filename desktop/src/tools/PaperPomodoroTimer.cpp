#include "tools/PaperPomodoroTimer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperPomodoroTimer::PaperPomodoroTimer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PomodoroTimer")
{
    timer_ = new QTimer(this);
    timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, &PaperPomodoroTimer::onTick);
    setupUI();
    loadSettings();
}

void PaperPomodoroTimer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    startBtn_ = new QPushButton("Start");
    startBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(startBtn_, &QPushButton::clicked, this, &PaperPomodoroTimer::onStart);
    toolbar->addWidget(startBtn_);

    stopBtn_ = new QPushButton("Stop");
    connect(stopBtn_, &QPushButton::clicked, this, &PaperPomodoroTimer::onStop);
    toolbar->addWidget(stopBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPomodoroTimer::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Focus with Pomodoro timer");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(560, 480);
}

void PaperPomodoroTimer::addSession(const PomodoroSession& session) {
    sessions_.append(session);
    saveSettings();
    updateInfo();
    update();
}

QList<PomodoroSession> PaperPomodoroTimer::sessions() const { return sessions_; }

QMap<QString, int> PaperPomodoroTimer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& s : sessions_) counts[s.category]++;
    return counts;
}

int PaperPomodoroTimer::totalMinutes() const {
    int t = 0;
    for (const auto& s : sessions_) if (s.completed) t += s.durationMinutes;
    return t;
}

int PaperPomodoroTimer::completedSessions() const {
    int c = 0;
    for (const auto& s : sessions_) if (s.completed) c++;
    return c;
}

void PaperPomodoroTimer::onStart() {
    bool ok;
    QString task = QInputDialog::getText(this, "Start Pomodoro", "Task:", QLineEdit::Normal, "", &ok);
    if (!ok || task.isEmpty()) return;
    QStringList cats = {"reading", "writing", "review", "analysis"};
    QString cat = QInputDialog::getItem(this, "Start Pomodoro", "Category:", cats, 0, false, &ok);
    if (!ok) return;

    PomodoroSession s;
    s.id = sessions_.size() + 1;
    s.task = task;
    s.durationMinutes = 25;
    s.elapsedSeconds = 0;
    s.completed = false;
    s.category = cat;

    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int cIdx = cats.indexOf(cat);
    s.color = catColors[qBound(0, cIdx, 3)];

    sessions_.append(s);
    currentSession_ = sessions_.size() - 1;
    currentElapsed_ = 0;
    timer_->start();
    updateInfo();
    update();
}

void PaperPomodoroTimer::onStop() {
    timer_->stop();
    if (currentSession_ >= 0 && currentSession_ < sessions_.size()) {
        sessions_[currentSession_].elapsedSeconds = currentElapsed_;
        sessions_[currentSession_].completed = currentElapsed_ >= sessions_[currentSession_].durationMinutes * 60;
    }
    currentSession_ = -1;
    currentElapsed_ = 0;
    saveSettings();
    updateInfo();
    update();
}

void PaperPomodoroTimer::onTick() {
    currentElapsed_++;
    if (currentSession_ >= 0 && currentSession_ < sessions_.size()) {
        emit timerTick(sessions_[currentSession_].durationMinutes * 60 - currentElapsed_);
        if (currentElapsed_ >= sessions_[currentSession_].durationMinutes * 60) {
            sessions_[currentSession_].elapsedSeconds = currentElapsed_;
            sessions_[currentSession_].completed = true;
            timer_->stop();
            emit sessionCompleted(sessions_[currentSession_].id);
            currentSession_ = -1;
            currentElapsed_ = 0;
            saveSettings();
            updateInfo();
        }
    }
    update();
}

void PaperPomodoroTimer::onClear() {
    sessions_.clear();
    currentSession_ = -1;
    currentElapsed_ = 0;
    timer_->stop();
    saveSettings();
    infoLabel_->setText("Focus with Pomodoro timer");
    update();
}

void PaperPomodoroTimer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (sessions_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Focus with Pomodoro timer");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Pomodoro Timer");

    int w = width(), h = height();
    drawTimerDial(p, QRect(20, 50, w / 2 - 20, h / 2));
    drawSessionList(p, QRect(20, h / 2 + 10, w - 40, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
}

void PaperPomodoroTimer::drawTimerDial(QPainter& p, const QRect& rect) {
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 20;

    p.setPen(QPen(QColor(241, 245, 249), 8));
    p.drawArc(cx - radius, cy - radius, radius * 2, radius * 2, 0, 360 * 16);

    qreal progress = 0;
    QColor activeColor = QColor(59, 130, 246);
    if (currentSession_ >= 0 && currentSession_ < sessions_.size()) {
        int total = sessions_[currentSession_].durationMinutes * 60;
        progress = static_cast<qreal>(currentElapsed_) / total;
        activeColor = sessions_[currentSession_].color;
    }

    p.setPen(QPen(activeColor, 8));
    int span = static_cast<int>(progress * 360 * 16);
    p.drawArc(cx - radius, cy - radius, radius * 2, radius * 2, 90 * 16, -span);

    int remaining = 0;
    if (currentSession_ >= 0 && currentSession_ < sessions_.size()) {
        remaining = sessions_[currentSession_].durationMinutes * 60 - currentElapsed_;
    }
    int mins = remaining / 60;
    int secs = remaining % 60;

    p.setPen(activeColor);
    p.setFont(QFont("Arial", 22, QFont::Bold));
    p.drawText(QRect(cx - 40, cy - 18, 80, 36), Qt::AlignCenter,
               QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0')));

    if (currentSession_ >= 0 && currentSession_ < sessions_.size()) {
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(QRect(cx - 40, cy + 18, 80, 16), Qt::AlignCenter,
                   sessions_[currentSession_].task.left(14));
    }
}

void PaperPomodoroTimer::drawSessionList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Sessions");

    int show = qMin(6, sessions_.size());
    int itemH = qMin(24, (rect.height() - 25) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& s = sessions_[sessions_.size() - 1 - i];
        int y = rect.y() + 20 + i * (itemH + 2);

        p.setPen(Qt::NoPen);
        p.setBrush(s.completed ? s.color : s.color.lighter(160));
        p.drawRoundedRect(rect.x(), y, 8, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 12, y, rect.width() / 2 - 12, itemH, Qt::AlignVCenter,
                   s.task.left(20));

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 2 - 10, itemH,
                   Qt::AlignVCenter | Qt::AlignRight,
                   s.completed ? QString::number(s.durationMinutes) + "min done" : "incomplete");
    }
}

void PaperPomodoroTimer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Sessions", QString::number(sessions_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completedSessions()), QColor(16,185,129)},
        {"Total Time", QString::number(totalMinutes()) + "min", QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperPomodoroTimer::updateInfo() {
    if (sessions_.isEmpty()) { infoLabel_->setText("Focus with Pomodoro timer"); return; }
    infoLabel_->setText(QString("%1 sessions | %2 done | %3min total")
        .arg(sessions_.size()).arg(completedSessions()).arg(totalMinutes()));
}

void PaperPomodoroTimer::loadSettings() {
    int size = settings_.beginReadArray("sessions");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PomodoroSession s;
        s.id = settings_.value("id").toInt();
        s.task = settings_.value("task").toString();
        s.durationMinutes = settings_.value("duration").toInt();
        s.elapsedSeconds = settings_.value("elapsed").toInt();
        s.completed = settings_.value("completed").toBool();
        s.category = settings_.value("category").toString();
        s.color = QColor(settings_.value("color").toString());
        sessions_.append(s);
    }
    settings_.endArray();
    updateInfo();
}

void PaperPomodoroTimer::saveSettings() {
    settings_.beginWriteArray("sessions");
    for (int i = 0; i < sessions_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", sessions_[i].id);
        settings_.setValue("task", sessions_[i].task);
        settings_.setValue("duration", sessions_[i].durationMinutes);
        settings_.setValue("elapsed", sessions_[i].elapsedSeconds);
        settings_.setValue("completed", sessions_[i].completed);
        settings_.setValue("category", sessions_[i].category);
        settings_.setValue("color", sessions_[i].color.name());
    }
    settings_.endArray();
}
