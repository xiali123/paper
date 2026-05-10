#include "reading/PaperReadingChallenge.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingChallenge::PaperReadingChallenge(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingChallenge")
{
    setupUI();
    loadSettings();
}

void PaperReadingChallenge::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    joinBtn_ = new QPushButton("Join");
    joinBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(joinBtn_, &QPushButton::clicked, this, &PaperReadingChallenge::onJoin);
    toolbar->addWidget(joinBtn_);

    toolbar->addWidget(new QLabel("Difficulty:"));
    difficultyCombo_ = new QComboBox();
    difficultyCombo_->addItems({"Easy", "Medium", "Hard", "Expert"});
    toolbar->addWidget(difficultyCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingChallenge::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter challenge name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Join reading challenges");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingChallenge::addEntry(const ReadingChallengeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit challengeJoined(entry.id, entry.progress);
    update();
}

QList<ReadingChallengeEntry> PaperReadingChallenge::entries() const { return entries_; }

int PaperReadingChallenge::completedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.completed) c++;
    return c;
}

qreal PaperReadingChallenge::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingChallenge::difficultyCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.difficulty]++;
    return counts;
}

void PaperReadingChallenge::onJoin() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList difficulties = {"easy", "medium", "hard", "expert"};
    QStringList durations = {"7 days", "14 days", "30 days", "90 days"};
    QStringList badges = {"Bronze", "Silver", "Gold", "Platinum"};

    int dIdx = difficultyCombo_->currentIndex();
    ReadingChallengeEntry e;
    e.id = entries_.size() + 1;
    e.challengeName = text.left(16);
    e.difficulty = difficulties[dIdx];
    e.targetPapers = (dIdx + 1) * 5 + QRandomGenerator::global()->bounded(10);
    e.completedPapers = QRandomGenerator::global()->bounded(e.targetPapers + 1);
    e.progress = static_cast<qreal>(e.completedPapers) / e.targetPapers;
    e.duration = durations[dIdx];
    e.participants = 1 + QRandomGenerator::global()->bounded(100);
    e.badge = badges[dIdx];
    e.completed = e.progress >= 1.0;
    e.color = e.completed ? QColor(16,185,129) : (e.progress >= 0.5 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperReadingChallenge::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Join reading challenges");
    update();
}

void PaperReadingChallenge::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Join reading challenges");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Challenges");

    int w = width(), h = height();
    drawChallengeList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDifficultyChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingChallenge::drawChallengeList(QPainter& p, const QRect& rect) {
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

        int barW = static_cast<int>(e.progress * (rect.width() - 12));
        p.setBrush(e.color.lighter(210));
        p.drawRoundedRect(rect.x() + 6, y + itemH - 6, barW, 3, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.challengeName.left(14) + (e.completed ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.difficulty + " | " + e.duration + " | " + e.badge);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.completedPapers) + "/" + QString::number(e.targetPapers));
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "% | " + QString::number(e.participants) + " users");
    }
}

void PaperReadingChallenge::drawDifficultyChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Difficulty");

    auto counts = difficultyCounts();
    QStringList diffs = {"easy", "medium", "hard", "expert"};
    QString labels[] = {"Easy", "Medium", "Hard", "Expert"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(diffs[i]) ? counts[diffs[i]] : 0;
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

void PaperReadingChallenge::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Challenges", QString::number(entries_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completedCount()), QColor(16,185,129)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Levels", QString::number(difficultyCounts().size()), QColor(139,92,246)}
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

void PaperReadingChallenge::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Join reading challenges"); return; }
    infoLabel_->setText(QString("%1 challenges | %2 done | %3% avg")
        .arg(entries_.size()).arg(completedCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingChallenge::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingChallengeEntry e;
        e.id = settings_.value("id").toInt();
        e.challengeName = settings_.value("challengeName").toString();
        e.difficulty = settings_.value("difficulty").toString();
        e.targetPapers = settings_.value("targetPapers").toInt();
        e.completedPapers = settings_.value("completedPapers").toInt();
        e.progress = settings_.value("progress").toDouble();
        e.duration = settings_.value("duration").toString();
        e.participants = settings_.value("participants").toInt();
        e.badge = settings_.value("badge").toString();
        e.completed = settings_.value("completed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingChallenge::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("challengeName", entries_[i].challengeName);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("targetPapers", entries_[i].targetPapers);
        settings_.setValue("completedPapers", entries_[i].completedPapers);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("participants", entries_[i].participants);
        settings_.setValue("badge", entries_[i].badge);
        settings_.setValue("completed", entries_[i].completed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
