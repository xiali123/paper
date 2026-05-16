#include "reading/ReadingTimerWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ReadingTimerWidget::ReadingTimerWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void ReadingTimerWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Paper label
    paperLabel_ = new QLabel("No paper selected");
    paperLabel_->setStyleSheet("font-size: 12px; color: #64748b; padding: 4px;");
    layout->addWidget(paperLabel_);

    // Timer face
    timerFace_ = new QWidget();
    timerFace_->setFixedSize(200, 200);
    timerFace_->setStyleSheet("background: transparent;");
    layout->addWidget(timerFace_, 0, Qt::AlignCenter);

    // Time display
    timeLabel_ = new QLabel("25:00");
    timeLabel_->setStyleSheet(
        "font-size: 48px; font-weight: bold; color: #3b82f6; padding: 10px;"
    );
    timeLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(timeLabel_);

    // Progress bar
    progressRing_ = new QProgressBar();
    progressRing_->setRange(0, 100);
    progressRing_->setValue(0);
    progressRing_->setTextVisible(false);
    progressRing_->setMaximumHeight(6);
    progressRing_->setStyleSheet(
        "QProgressBar { background: #e2e8f0; border-radius: 3px; }"
        "QProgressBar::chunk { background: #3b82f6; border-radius: 3px; }"
    );
    layout->addWidget(progressRing_);

    // Controls
    auto* ctrlRow = new QHBoxLayout();

    durationCombo_ = new QComboBox();
    durationCombo_->addItems({"15 min", "25 min", "30 min", "45 min", "60 min", "90 min"});
    durationCombo_->setCurrentIndex(1);
    connect(durationCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingTimerWidget::onDurationChanged);
    ctrlRow->addWidget(durationCombo_);

    startPauseBtn_ = new QPushButton("Start");
    startPauseBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; font-size: 14px; }"
    );
    connect(startPauseBtn_, &QPushButton::clicked, this, &ReadingTimerWidget::onStartPause);
    ctrlRow->addWidget(startPauseBtn_);

    resetBtn_ = new QPushButton("Reset");
    connect(resetBtn_, &QPushButton::clicked, this, &ReadingTimerWidget::onReset);
    ctrlRow->addWidget(resetBtn_);

    layout->addLayout(ctrlRow);

    // Stats
    statsLabel_ = new QLabel("0 sessions | 0 min total");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    // History
    auto* histRow = new QHBoxLayout();
    histRow->addWidget(new QLabel("History:"), 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &ReadingTimerWidget::onClearHistory);
    histRow->addWidget(clearBtn_);
    layout->addLayout(histRow);

    historyList_ = new QListWidget();
    historyList_->setMaximumHeight(150);
    historyList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
    );
    layout->addWidget(historyList_, 1);

    // Tick timer
    tickTimer_ = new QTimer(this);
    tickTimer_->setInterval(1000);
    connect(tickTimer_, &QTimer::timeout, this, &ReadingTimerWidget::onTick);
}

void ReadingTimerWidget::setPaper(const QString& title) {
    currentPaper_ = title;
    paperLabel_->setText(title.isEmpty() ? "No paper selected" : "Reading: " + title);
}

void ReadingTimerWidget::startTimer() {
    running_ = true;
    tickTimer_->start();
    startPauseBtn_->setText("Pause");
    startPauseBtn_->setStyleSheet(
        "QPushButton { background: #f59e0b; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; font-size: 14px; }"
    );
    emit timerStarted();
}

void ReadingTimerWidget::pauseTimer() {
    running_ = false;
    tickTimer_->stop();
    startPauseBtn_->setText("Resume");
    startPauseBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; font-size: 14px; }"
    );
    emit timerPaused();
}

void ReadingTimerWidget::resetTimer() {
    running_ = false;
    tickTimer_->stop();
    elapsedSec_ = 0;
    startPauseBtn_->setText("Start");
    startPauseBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; font-size: 14px; }"
    );
    updateDisplay();
    emit timerReset();
}

void ReadingTimerWidget::setDuration(int minutes) {
    targetSec_ = minutes * 60;
    elapsedSec_ = 0;
    updateDisplay();
}

QList<ReadingSession> ReadingTimerWidget::sessions() const { return sessions_; }

int ReadingTimerWidget::totalReadingMinutes() const {
    int total = 0;
    for (const auto& s : sessions_) total += s.durationSec;
    return total / 60;
}

int ReadingTimerWidget::todayReadingMinutes() const {
    qint64 todayStart = QDateTime::currentDateTime().toSecsSinceEpoch() -
                        QTime(0,0).secsTo(QTime::currentTime());
    int total = 0;
    for (const auto& s : sessions_) {
        if (s.timestamp >= todayStart) total += s.durationSec;
    }
    return total / 60;
}

void ReadingTimerWidget::onTick() {
    elapsedSec_++;
    updateDisplay();

    if (elapsedSec_ >= targetSec_) {
        tickTimer_->stop();
        running_ = false;
        startPauseBtn_->setText("Start");
        startPauseBtn_->setStyleSheet(
            "QPushButton { background: #059669; color: white; padding: 8px 24px; "
            "border-radius: 6px; font-weight: bold; font-size: 14px; }"
        );

        ReadingSession session;
        session.id = nextSessionId_++;
        session.paperTitle = currentPaper_;
        session.durationSec = elapsedSec_;
        session.targetSec = targetSec_;
        session.timestamp = QDateTime::currentSecsSinceEpoch();
        session.completed = true;
        sessions_.append(session);
        saveSettings();
        refreshHistory();
        emit timerCompleted(elapsedSec_);
        emit sessionSaved(session);
    }
}

void ReadingTimerWidget::onStartPause() {
    if (running_) pauseTimer();
    else startTimer();
}

void ReadingTimerWidget::onReset() { resetTimer(); }

void ReadingTimerWidget::onDurationChanged(int index) {
    QList<int> mins = {15, 25, 30, 45, 60, 90};
    if (index >= 0 && index < mins.size()) {
        setDuration(mins[index]);
    }
}

void ReadingTimerWidget::onClearHistory() {
    sessions_.clear();
    saveSettings();
    refreshHistory();
}

void ReadingTimerWidget::updateDisplay() {
    int remaining = qMax(0, targetSec_ - elapsedSec_);
    int min = remaining / 60;
    int sec = remaining % 60;
    timeLabel_->setText(QString("%1:%2")
        .arg(min, 2, 10, QChar('0'))
        .arg(sec, 2, 10, QChar('0')));

    int pct = (targetSec_ > 0) ? static_cast<int>(elapsedSec_ * 100.0 / targetSec_) : 0;
    progressRing_->setValue(qMin(100, pct));

    update();
}

void ReadingTimerWidget::paintEvent(QPaintEvent*) {
    if (!timerFace_) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect fr = timerFace_->geometry();
    int cx = fr.center().x();
    int cy = fr.center().y();
    int r = qMin(fr.width(), fr.height()) / 2 - 6;
    int sz = timerFace_->width();
    int cx = sz / 2, cy = sz / 2, r = sz / 2 - 10;

    p.setRenderHint(QPainter::Antialiasing);

    // Background circle
    QPen bgPen(QColor(226, 232, 240), 6);
    p.setPen(bgPen);
    p.drawEllipse(cx - r, cy - r, r * 2, r * 2);

    // Progress arc
    qreal pct = (targetSec_ > 0) ? static_cast<qreal>(elapsedSec_) / targetSec_ : 0;
    if (pct > 0) {
        QColor color = (pct >= 1.0) ? QColor(5, 150, 105) : QColor(59, 130, 246);
        QPen fgPen(color, 6);
        fgPen.setCapStyle(Qt::RoundCap);
        p.setPen(fgPen);
        int startAngle = 90 * 16;
        int spanAngle = -static_cast<int>(pct * 360 * 16);
        p.drawArc(cx - r, cy - r, r * 2, r * 2, startAngle, spanAngle);
    }

    // Center text
    p.setPen(QColor(51, 65, 85));
    QFont font = p.font();
    font.setPixelSize(14);
    font.setBold(true);
    p.setFont(font);
    int remaining = qMax(0, targetSec_ - elapsedSec_);
    p.drawText(QRect(cx - r, cy - 10, r * 2, 20), Qt::AlignCenter,
               QString("%1:%2")
                   .arg(remaining / 60, 2, 10, QChar('0'))
                   .arg(remaining % 60, 2, 10, QChar('0')));
}

void ReadingTimerWidget::refreshHistory() {
    historyList_->clear();
    for (int i = sessions_.size() - 1; i >= 0; --i) {
        const auto& s = sessions_[i];
        QDateTime dt = QDateTime::fromSecsSinceEpoch(s.timestamp);
        QString label = QString("%1 | %2 min | %3")
            .arg(dt.toString("MM-dd HH:mm"))
            .arg(s.durationSec / 60)
            .arg(s.paperTitle.isEmpty() ? "Free reading" : s.paperTitle);
        if (!s.completed) label += " (partial)";
        auto* item = new QListWidgetItem(label);
        if (s.completed) item->setForeground(QColor(5, 150, 105));
        historyList_->addItem(item);
    }
    statsLabel_->setText(QString("%1 sessions | %2 min total | %3 min today")
        .arg(sessions_.size()).arg(totalReadingMinutes()).arg(todayReadingMinutes()));
}

void ReadingTimerWidget::loadSettings() {
    QSettings settings("PaperCrawler", "ReadingTimer");
    QByteArray data = settings.value("sessions").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ReadingSession s;
        s.id = obj["id"].toInt();
        s.paperTitle = obj["paperTitle"].toString();
        s.durationSec = obj["durationSec"].toInt();
        s.targetSec = obj["targetSec"].toInt();
        s.timestamp = obj["timestamp"].toInteger();
        s.completed = obj["completed"].toBool();
        sessions_.append(s);
        nextSessionId_ = qMax(nextSessionId_, s.id + 1);
    }
    refreshHistory();
}

void ReadingTimerWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& s : sessions_) {
        QJsonObject obj;
        obj["id"] = s.id;
        obj["paperTitle"] = s.paperTitle;
        obj["durationSec"] = s.durationSec;
        obj["targetSec"] = s.targetSec;
        obj["timestamp"] = static_cast<qint64>(s.timestamp);
        obj["completed"] = s.completed;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ReadingTimer");
    settings.setValue("sessions", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
