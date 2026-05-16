#include "reading/PaperReadingSpeedTest.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingSpeedTest::PaperReadingSpeedTest(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingSpeedTest")
{
    setupUI();
    loadSettings();
}

void PaperReadingSpeedTest::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    testBtn_ = new QPushButton("Test");
    testBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(testBtn_, &QPushButton::clicked, this, &PaperReadingSpeedTest::onTest);
    toolbar->addWidget(testBtn_);

    toolbar->addWidget(new QLabel("Difficulty:"));
    difficultyCombo_ = new QComboBox();
    difficultyCombo_->addItems({"Easy", "Medium", "Hard"});
    toolbar->addWidget(difficultyCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSpeedTest::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title for speed test...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Test reading speed");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingSpeedTest::addEntry(const SpeedTestEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit testCompleted(entry.id, entry.wpm);
    update();
}

QList<SpeedTestEntry> PaperReadingSpeedTest::entries() const { return entries_; }

qreal PaperReadingSpeedTest::avgWPM() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.wpm;
    return sum / entries_.size();
}

int PaperReadingSpeedTest::passedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.passedBaseline) c++;
    return c;
}

QMap<QString, int> PaperReadingSpeedTest::difficultyCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.difficulty]++;
    return counts;
}

void PaperReadingSpeedTest::onTest() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList difficulties = {"easy", "medium", "hard"};
    QStringList languages = {"English", "Chinese", "Mixed"};
    QStringList genres = {"survey", "empirical", "theoretical", "case-study"};

    int dIdx = difficultyCombo_->currentIndex();
    SpeedTestEntry e;
    e.id = entries_.size() + 1;
    e.paperTitle = text.left(15);
    e.wordsRead = 500 + QRandomGenerator::global()->bounded(3000);
    int baseTime = 60 + QRandomGenerator::global()->bounded(300);
    e.timeSeconds = baseTime * (1 + dIdx * 0.3);
    e.wpm = e.wordsRead / (e.timeSeconds / 60.0);
    e.comprehension = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
    e.difficulty = difficulties[dIdx];
    e.language = languages[QRandomGenerator::global()->bounded(languages.size())];
    e.genre = genres[QRandomGenerator::global()->bounded(genres.size())];
    e.passedBaseline = e.wpm >= 200 && e.comprehension >= 0.6;
    e.color = e.passedBaseline ? QColor(16,185,129) : (e.wpm >= 150 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperReadingSpeedTest::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Test reading speed");
    update();
}

void PaperReadingSpeedTest::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Test reading speed");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Speed Test");

    int w = width(), h = height();
    drawResultList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDifficultyChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingSpeedTest::drawResultList(QPainter& p, const QRect& rect) {
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
                   e.paperTitle.left(14) + " [" + e.difficulty.left(1).toUpper() + "]");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.language + " | " + e.genre);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(static_cast<int>(e.wpm)) + " WPM");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.comprehension * 100, 'f', 0) + "% comp | " + QString::number(e.timeSeconds) + "s");
    }
}

void PaperReadingSpeedTest::drawDifficultyChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Difficulty");

    auto counts = difficultyCounts();
    QStringList diffs = {"easy", "medium", "hard"};
    QString labels[] = {"Easy", "Medium", "Hard"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(diffs[i]) ? counts[diffs[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperReadingSpeedTest::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tests", QString::number(entries_.size()), QColor(59,130,246)},
        {"Avg WPM", QString::number(static_cast<int>(avgWPM())), QColor(16,185,129)},
        {"Passed", QString::number(passedCount()), QColor(245,158,11)},
        {"Difficulties", QString::number(difficultyCounts().size()), QColor(139,92,246)}
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

void PaperReadingSpeedTest::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Test reading speed"); return; }
    infoLabel_->setText(QString("%1 tests | %2 avg WPM | %3 passed")
        .arg(entries_.size()).arg(static_cast<int>(avgWPM())).arg(passedCount()));
}

void PaperReadingSpeedTest::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SpeedTestEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.wordsRead = settings_.value("wordsRead").toInt();
        e.timeSeconds = settings_.value("timeSeconds").toInt();
        e.wpm = settings_.value("wpm").toDouble();
        e.comprehension = settings_.value("comprehension").toDouble();
        e.difficulty = settings_.value("difficulty").toString();
        e.language = settings_.value("language").toString();
        e.genre = settings_.value("genre").toString();
        e.passedBaseline = settings_.value("passedBaseline").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingSpeedTest::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("wordsRead", entries_[i].wordsRead);
        settings_.setValue("timeSeconds", entries_[i].timeSeconds);
        settings_.setValue("wpm", entries_[i].wpm);
        settings_.setValue("comprehension", entries_[i].comprehension);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("language", entries_[i].language);
        settings_.setValue("genre", entries_[i].genre);
        settings_.setValue("passedBaseline", entries_[i].passedBaseline);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
