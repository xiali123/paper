#include "reading/PaperReadingSessionTimer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingSessionTimer::PaperReadingSessionTimer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingSessionTimer")
{
    setupUI();
    loadSettings();
}

void PaperReadingSessionTimer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    logBtn_ = new QPushButton("Log Session");
    logBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(logBtn_, &QPushButton::clicked, this, &PaperReadingSessionTimer::onLog);
    toolbar->addWidget(logBtn_);

    toolbar->addWidget(new QLabel("Activity:"));
    activityCombo_ = new QComboBox();
    activityCombo_->addItems({"Deep Read", "Skimming", "Note-taking", "Review", "Reference"});
    toolbar->addWidget(activityCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSessionTimer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title for reading session...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track reading sessions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingSessionTimer::addEntry(const SessionTimerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sessionLogged(entry.id, entry.durationMin);
    update();
}

QList<SessionTimerEntry> PaperReadingSessionTimer::entries() const { return entries_; }

int PaperReadingSessionTimer::totalMinutes() const {
    int t = 0;
    for (const auto& e : entries_) t += e.durationMin;
    return t;
}

qreal PaperReadingSessionTimer::avgFocus() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.focusScore;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingSessionTimer::activityCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.activity]++;
    return counts;
}

void PaperReadingSessionTimer::onLog() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList activities = {"deep-read", "skimming", "note-taking", "review", "reference"};
    QStringList devices = {"Desktop", "Tablet", "Print"};
    QStringList moods = {"focused", "distracted", "tired", "curious"};

    int aIdx = activityCombo_->currentIndex();
    SessionTimerEntry e;
    e.id = entries_.size() + 1;
    e.paperTitle = text.left(15);
    e.durationMin = 10 + QRandomGenerator::global()->bounded(110);
    e.focusScore = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.activity = activities[aIdx];
    e.pagesRead = 1 + QRandomGenerator::global()->bounded(25);
    e.comprehension = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.device = devices[QRandomGenerator::global()->bounded(devices.size())];
    e.mood = moods[QRandomGenerator::global()->bounded(moods.size())];
    e.color = e.focusScore >= 0.7 ? QColor(16,185,129) : (e.focusScore >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperReadingSessionTimer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track reading sessions");
    update();
}

void PaperReadingSessionTimer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading sessions");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Session Timer");

    int w = width(), h = height();
    drawSessionList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawActivityChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingSessionTimer::drawSessionList(QPainter& p, const QRect& rect) {
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
                   e.paperTitle.left(14) + " [" + e.activity.left(6) + "]");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.device + " | " + e.mood);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.durationMin) + "min | " + QString::number(e.pagesRead) + "pg");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.focusScore * 100, 'f', 0) + "% focus | " + QString::number(e.comprehension * 100, 'f', 0) + "% comp");
    }
}

void PaperReadingSessionTimer::drawActivityChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Activities");

    auto counts = activityCounts();
    QStringList acts = {"deep-read", "skimming", "note-taking", "review", "reference"};
    QString labels[] = {"Deep", "Skim", "Notes", "Review", "Ref"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(acts[i]) ? counts[acts[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingSessionTimer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Sessions", QString::number(entries_.size()), QColor(59,130,246)},
        {"Total Time", QString::number(totalMinutes()) + "min", QColor(16,185,129)},
        {"Avg Focus", QString::number(avgFocus() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Activities", QString::number(activityCounts().size()), QColor(139,92,246)}
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

void PaperReadingSessionTimer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track reading sessions"); return; }
    infoLabel_->setText(QString("%1 sessions | %2min total | %3% focus")
        .arg(entries_.size()).arg(totalMinutes()).arg(avgFocus() * 100, 0, 'f', 0));
}

void PaperReadingSessionTimer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SessionTimerEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.durationMin = settings_.value("durationMin").toInt();
        e.focusScore = settings_.value("focusScore").toDouble();
        e.activity = settings_.value("activity").toString();
        e.pagesRead = settings_.value("pagesRead").toInt();
        e.comprehension = settings_.value("comprehension").toDouble();
        e.device = settings_.value("device").toString();
        e.mood = settings_.value("mood").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingSessionTimer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("durationMin", entries_[i].durationMin);
        settings_.setValue("focusScore", entries_[i].focusScore);
        settings_.setValue("activity", entries_[i].activity);
        settings_.setValue("pagesRead", entries_[i].pagesRead);
        settings_.setValue("comprehension", entries_[i].comprehension);
        settings_.setValue("device", entries_[i].device);
        settings_.setValue("mood", entries_[i].mood);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
