#include "reading/ReadingSessionLog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QHeaderView>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ReadingSessionLog::ReadingSessionLog(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();

    tickTimer_ = new QTimer(this);
    connect(tickTimer_, &QTimer::timeout, this, &ReadingSessionLog::onTimerTick);
}

void ReadingSessionLog::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Timer display
    timerLabel_ = new QLabel("00:00:00");
    timerLabel_->setStyleSheet("font-size: 28px; font-weight: bold; color: #3b82f6; padding: 8px;");
    timerLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(timerLabel_);

    // Controls
    auto* ctrlRow = new QHBoxLayout();
    startBtn_ = new QPushButton("Start Session");
    startBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 6px 16px; border-radius: 6px; font-weight: bold; }");
    connect(startBtn_, &QPushButton::clicked, this, &ReadingSessionLog::onStart);
    ctrlRow->addWidget(startBtn_);

    stopBtn_ = new QPushButton("Stop Session");
    stopBtn_->setStyleSheet("QPushButton { background: #dc2626; color: white; padding: 6px 16px; border-radius: 6px; font-weight: bold; }");
    stopBtn_->setEnabled(false);
    connect(stopBtn_, &QPushButton::clicked, this, &ReadingSessionLog::onStop);
    ctrlRow->addWidget(stopBtn_);

    ctrlRow->addWidget(new QLabel("Activity:"));
    activityCombo_ = new QComboBox();
    activityCombo_->addItems({"Reading", "Skimming", "Note-taking", "Reviewing", "Citing", "Comparing"});
    ctrlRow->addWidget(activityCombo_, 1);
    layout->addLayout(ctrlRow);

    // Notes
    notesEdit_ = new QTextEdit();
    notesEdit_->setMaximumHeight(50);
    notesEdit_->setPlaceholderText("Session notes...");
    layout->addWidget(notesEdit_);

    // Filter
    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Today", "This Week", "Reading", "Skimming", "Reviewing"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingSessionLog::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &ReadingSessionLog::onExport);
    filterRow->addWidget(exportBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &ReadingSessionLog::onDelete);
    filterRow->addWidget(deleteBtn_);
    layout->addLayout(filterRow);

    // Session tree
    sessionTree_ = new QTreeWidget();
    sessionTree_->setHeaderLabels({"Paper", "Date", "Duration", "Activity", "Focus"});
    sessionTree_->setColumnWidth(0, 180);
    sessionTree_->setColumnWidth(1, 80);
    sessionTree_->setColumnWidth(2, 70);
    sessionTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    layout->addWidget(sessionTree_, 1);

    statsLabel_ = new QLabel("0 sessions");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void ReadingSessionLog::startSession(int paperId, const QString& title) {
    currentPaperId_ = paperId;
    currentTitle_ = title;
    elapsed_.start();
    sessionActive_ = true;
    startBtn_->setEnabled(false);
    stopBtn_->setEnabled(true);
    tickTimer_->start(1000);
    emit sessionStarted(paperId);
}

void ReadingSessionLog::stopSession() {
    if (!sessionActive_) return;
    tickTimer_->stop();
    sessionActive_ = false;
    startBtn_->setEnabled(true);
    stopBtn_->setEnabled(false);

    SessionEntry e;
    e.paperId = currentPaperId_;
    e.paperTitle = currentTitle_;
    e.date = QDate::currentDate();
    e.startTime = QDateTime::currentSecsSinceEpoch() - elapsed_.elapsed() / 1000;
    e.endTime = QDateTime::currentSecsSinceEpoch();
    e.durationSec = elapsed_.elapsed() / 1000;
    e.activity = activityCombo_->currentText();
    e.notes = notesEdit_->toPlainText();
    e.focusScore = qBound(1, e.durationSec / 60, 10);
    addEntry(e);

    emit sessionStopped(currentPaperId_, e.durationSec);
    notesEdit_->clear();
    timerLabel_->setText("00:00:00");
}

void ReadingSessionLog::addEntry(const SessionEntry& entry) {
    SessionEntry e = entry;
    if (e.id < 0) e.id = nextId_++;
    entries_.append(e);
    nextId_ = qMax(nextId_, e.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
    emit entryAdded(e.id);
}

void ReadingSessionLog::removeEntry(int entryId) {
    entries_.removeIf([entryId](const SessionEntry& e) { return e.id == entryId; });
    refreshTree();
    saveSettings();
    updateStats();
}

QList<SessionEntry> ReadingSessionLog::entries() const { return entries_; }

QList<SessionEntry> ReadingSessionLog::entriesForDate(const QDate& date) const {
    QList<SessionEntry> result;
    for (const auto& e : entries_) {
        if (e.date == date) result.append(e);
    }
    return result;
}

int ReadingSessionLog::totalMinutesThisWeek() const {
    int total = 0;
    QDate weekStart = QDate::currentDate().addDays(-7);
    for (const auto& e : entries_) {
        if (e.date >= weekStart) total += e.durationSec;
    }
    return total / 60;
}

void ReadingSessionLog::onStart() {
    startSession(0, "Manual Session");
}

void ReadingSessionLog::onStop() { stopSession(); }

void ReadingSessionLog::onDelete() {
    auto* item = sessionTree_->currentItem();
    if (!item) return;
    int id = item->data(0, Qt::UserRole).toInt();
    removeEntry(id);
}

void ReadingSessionLog::onFilterChanged(int) { refreshTree(); }

void ReadingSessionLog::onTimerTick() {
    if (!sessionActive_) return;
    updateTimerDisplay();
}

void ReadingSessionLog::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Sessions", "", "JSON (*.json)");
    if (path.isEmpty()) return;
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["paperId"] = e.paperId;
        obj["paperTitle"] = e.paperTitle;
        obj["date"] = e.date.toString(Qt::ISODate);
        obj["durationSec"] = e.durationSec;
        obj["activity"] = e.activity;
        obj["notes"] = e.notes;
        obj["focusScore"] = e.focusScore;
        arr.append(obj);
    }
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(arr).toJson());
}

void ReadingSessionLog::refreshTree() {
    sessionTree_->clear();
    int filter = filterCombo_->currentIndex();
    QDate today = QDate::currentDate();
    QDate weekStart = today.addDays(-7);

    for (const auto& e : entries_) {
        if (filter == 1 && e.date != today) continue;
        if (filter == 2 && e.date < weekStart) continue;
        if (filter == 3 && e.activity != "Reading") continue;
        if (filter == 4 && e.activity != "Skimming") continue;
        if (filter == 5 && e.activity != "Reviewing") continue;

        int mins = e.durationSec / 60;
        int secs = e.durationSec % 60;
        QString duration = QString("%1m %2s").arg(mins).arg(secs);

        auto* item = new QTreeWidgetItem({
            e.paperTitle.left(25),
            e.date.toString("MM-dd"),
            duration,
            e.activity,
            QString::number(e.focusScore)
        });
        item->setData(0, Qt::UserRole, e.id);

        if (e.focusScore >= 7) item->setForeground(4, QColor(16, 185, 129));
        else if (e.focusScore >= 4) item->setForeground(4, QColor(59, 130, 246));
        else item->setForeground(4, QColor(245, 158, 11));

        sessionTree_->addTopLevelItem(item);
    }
}

void ReadingSessionLog::updateStats() {
    int totalSecs = 0, sessions = entries_.size();
    for (const auto& e : entries_) totalSecs += e.durationSec;
    int totalMins = totalSecs / 60;
    statsLabel_->setText(QString("%1 sessions | %2 min total | %3 min this week")
        .arg(sessions).arg(totalMins).arg(totalMinutesThisWeek()));
}

void ReadingSessionLog::updateTimerDisplay() {
    qint64 secs = elapsed_.elapsed() / 1000;
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = secs % 60;
    timerLabel_->setText(QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0')));
}

void ReadingSessionLog::loadSettings() {
    QSettings settings("PaperCrawler", "SessionLog");
    QByteArray data = settings.value("entries").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        SessionEntry e;
        e.id = obj["id"].toInt();
        e.paperId = obj["paperId"].toInt();
        e.paperTitle = obj["paperTitle"].toString();
        e.date = QDate::fromString(obj["date"].toString(), Qt::ISODate);
        e.startTime = obj["startTime"].toInteger();
        e.endTime = obj["endTime"].toInteger();
        e.durationSec = obj["durationSec"].toInt();
        e.activity = obj["activity"].toString();
        e.notes = obj["notes"].toString();
        e.focusScore = obj["focusScore"].toInt();
        entries_.append(e);
        nextId_ = qMax(nextId_, e.id + 1);
    }
    refreshTree();
    updateStats();
}

void ReadingSessionLog::saveSettings() {
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["paperId"] = e.paperId;
        obj["paperTitle"] = e.paperTitle;
        obj["date"] = e.date.toString(Qt::ISODate);
        obj["startTime"] = static_cast<qint64>(e.startTime);
        obj["endTime"] = static_cast<qint64>(e.endTime);
        obj["durationSec"] = e.durationSec;
        obj["activity"] = e.activity;
        obj["notes"] = e.notes;
        obj["focusScore"] = e.focusScore;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "SessionLog");
    settings.setValue("entries", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
