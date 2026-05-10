#include "reading/PaperReadingPuzzle.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingPuzzle::PaperReadingPuzzle(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingPuzzle")
{
    setupUI();
    loadSettings();
}

void PaperReadingPuzzle::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    solveBtn_ = new QPushButton("Solve");
    solveBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(solveBtn_, &QPushButton::clicked, this, &PaperReadingPuzzle::onSolve);
    toolbar->addWidget(solveBtn_);

    toolbar->addWidget(new QLabel("Topic:"));
    topicCombo_ = new QComboBox();
    topicCombo_->addItems({"ML", "NLP", "CV", "Statistics", "Theory"});
    toolbar->addWidget(topicCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingPuzzle::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter puzzle name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Solve reading puzzles");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingPuzzle::addEntry(const PuzzleEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit puzzleSolved(entry.id, entry.score);
    update();
}

QList<PuzzleEntry> PaperReadingPuzzle::entries() const { return entries_; }

int PaperReadingPuzzle::passedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.passed) c++;
    return c;
}

qreal PaperReadingPuzzle::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingPuzzle::topicCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.topic]++;
    return counts;
}

void PaperReadingPuzzle::onSolve() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList topics = {"ML", "NLP", "CV", "Statistics", "Theory"};
    QStringList difficulties = {"easy", "medium", "hard"};
    QStringList badges = {"Bronze", "Silver", "Gold"};

    int tIdx = topicCombo_->currentIndex();
    PuzzleEntry e;
    e.id = entries_.size() + 1;
    e.puzzleName = text.left(14);
    e.topic = topics[tIdx];
    int dIdx = QRandomGenerator::global()->bounded(difficulties.size());
    e.difficulty = difficulties[dIdx];
    e.questions = 5 + QRandomGenerator::global()->bounded(15);
    e.correct = QRandomGenerator::global()->bounded(e.questions + 1);
    e.score = static_cast<qreal>(e.correct) / e.questions;
    e.timeSeconds = 30 + QRandomGenerator::global()->bounded(300);
    e.badge = e.score >= 0.9 ? "Gold" : (e.score >= 0.7 ? "Silver" : "Bronze");
    e.passed = e.score >= 0.6;
    e.color = e.passed ? QColor(16,185,129) : (e.score >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperReadingPuzzle::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Solve reading puzzles");
    update();
}

void PaperReadingPuzzle::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Solve reading puzzles");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Puzzle");

    int w = width(), h = height();
    drawPuzzleList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTopicChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingPuzzle::drawPuzzleList(QPainter& p, const QRect& rect) {
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

        int barW = static_cast<int>(e.score * (rect.width() - 12));
        p.setBrush(e.color.lighter(210));
        p.drawRoundedRect(rect.x() + 6, y + itemH - 6, barW, 3, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.puzzleName.left(14) + (e.passed ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.difficulty + " | " + e.topic + " | " + e.badge);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.correct) + "/" + QString::number(e.questions));
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.score * 100, 'f', 0) + "% | " + QString::number(e.timeSeconds) + "s");
    }
}

void PaperReadingPuzzle::drawTopicChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Topics");

    auto counts = topicCounts();
    QStringList topics = {"ML", "NLP", "CV", "Statistics", "Theory"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(topics[i]) ? counts[topics[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, topics[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingPuzzle::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Puzzles", QString::number(entries_.size()), QColor(59,130,246)},
        {"Passed", QString::number(passedCount()), QColor(16,185,129)},
        {"Avg Score", QString::number(avgScore() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Topics", QString::number(topicCounts().size()), QColor(139,92,246)}
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

void PaperReadingPuzzle::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Solve reading puzzles"); return; }
    infoLabel_->setText(QString("%1 puzzles | %2 passed | %3% avg")
        .arg(entries_.size()).arg(passedCount()).arg(avgScore() * 100, 0, 'f', 0));
}

void PaperReadingPuzzle::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PuzzleEntry e;
        e.id = settings_.value("id").toInt();
        e.puzzleName = settings_.value("puzzleName").toString();
        e.difficulty = settings_.value("difficulty").toString();
        e.questions = settings_.value("questions").toInt();
        e.correct = settings_.value("correct").toInt();
        e.score = settings_.value("score").toDouble();
        e.topic = settings_.value("topic").toString();
        e.timeSeconds = settings_.value("timeSeconds").toInt();
        e.badge = settings_.value("badge").toString();
        e.passed = settings_.value("passed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingPuzzle::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("puzzleName", entries_[i].puzzleName);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("questions", entries_[i].questions);
        settings_.setValue("correct", entries_[i].correct);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("timeSeconds", entries_[i].timeSeconds);
        settings_.setValue("badge", entries_[i].badge);
        settings_.setValue("passed", entries_[i].passed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
