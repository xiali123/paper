#include "reading/PaperQuizGenerator.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QRandomGenerator>
#include <QScrollBar>

PaperQuizGenerator::PaperQuizGenerator(QWidget *parent)
    : QWidget(parent)
    , nextId_(1)
{
    setupUI();
    loadSettings();
}

void PaperQuizGenerator::setupUI()
{
    setMinimumSize(580, 480);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Toolbar
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    generateBtn_ = new QPushButton(tr("Generate"));
    generateBtn_->setStyleSheet(
        "QPushButton { background-color: #3b82f6; color: white; border: none; "
        "border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperQuizGenerator::onGenerate);
    toolbar->addWidget(generateBtn_);

    difficultyCombo_ = new QComboBox;
    difficultyCombo_->addItems({tr("All"), tr("Easy"), tr("Medium"), tr("Hard"), tr("Mixed")});
    difficultyCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; "
        "min-width: 120px; }");
    toolbar->addWidget(difficultyCombo_);

    topicEdit_ = new QLineEdit;
    topicEdit_->setPlaceholderText(tr("Enter quiz topic..."));
    topicEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; }");
    toolbar->addWidget(topicEdit_, 1);

    infoLabel_ = new QLabel(tr("Generate quizzes"));
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 12px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);

    // Scroll area
    scrollArea_ = new QScrollArea;
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet("QScrollArea { border: none; background: white; }");
    contentWidget_ = new QWidget;
    contentWidget_->setStyleSheet("background: white;");
    contentLayout_ = new QVBoxLayout(contentWidget_);
    contentLayout_->setSpacing(10);
    contentLayout_->setContentsMargins(4, 4, 4, 4);
    scrollArea_->setWidget(contentWidget_);
    mainLayout->addWidget(scrollArea_, 1);
}

void PaperQuizGenerator::onGenerate()
{
    const QStringList categories = {"easy", "medium", "hard"};
    const QStringList difficulties = {"easy", "medium", "hard"};
    const QString text = topicEdit_->text().trimmed().isEmpty()
        ? "topic" : topicEdit_->text().trimmed();

    entries_.clear();
    const int count = 4 + QRandomGenerator::global()->bounded(5); // 4-8

    for (int i = 0; i < count; ++i) {
        QuizEntry e;
        e.id = nextId_++;
        e.category = categories.at(QRandomGenerator::global()->bounded(categories.size()));
        e.difficulty = difficulties.at(QRandomGenerator::global()->bounded(difficulties.size()));
        e.question = QString("Q%1: What is %2 concept %3?")
            .arg(i + 1).arg(text).arg(i + 1);
        e.answer = QString("Answer %1 for %2").arg(i + 1).arg(text);
        e.score = QRandomGenerator::global()->bounded(100) / 100.0;
        e.correct = e.score >= 0.6;
        e.color = e.correct ? QColor("#16a34a") : QColor("#239,68,68");
        entries_.append(e);
        emit quizGenerated(e.id, e.score);
    }

    updateInfo();
    update();
}

void PaperQuizGenerator::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Generate quizzes"));
        return;
    }
    const int correctCount = std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const QuizEntry &e) { return e.correct; });
    double avgScore = 0;
    for (const auto &e : entries_) avgScore += e.score;
    avgScore = (avgScore / entries_.size()) * 100.0;

    infoLabel_->setText(tr("%1 questions | %2 correct | %3% avg")
        .arg(entries_.size()).arg(correctCount).arg(avgScore, 0, 'f', 1));
}

void PaperQuizGenerator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    drawQuizList(p);
    drawCategoryChart(p);
    drawStats(p);
}

void PaperQuizGenerator::drawQuizList(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int y = 60;
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("Quiz Questions"));
    y += 24;

    p.setFont(QFont("Sans", 10));
    for (const auto &e : entries_) {
        if (y > height() - 120) break;

        // Background card
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f9fafb"));
        p.drawRoundedRect(10, y - 14, width() - 20, 44, 6, 6);

        // Color indicator
        p.setBrush(e.color);
        p.drawRoundedRect(14, y - 6, 6, 28, 3, 3);

        // Question text
        p.setPen(QColor("#1f2937"));
        p.drawText(28, y + 2, e.question);

        // Difficulty badge
        QColor diffColor;
        if (e.difficulty == "easy") diffColor = QColor("#16a34a");
        else if (e.difficulty == "medium") diffColor = QColor("#245,158,11");
        else diffColor = QColor("#239,68,68");
        p.setPen(Qt::NoPen);
        p.setBrush(diffColor);
        p.drawRoundedRect(340, y - 8, 60, 18, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Sans", 8, QFont::Bold));
        p.drawText(345, y + 5, e.difficulty.toUpper());

        // Score and correctness
        p.setFont(QFont("Sans", 9));
        p.setPen(QColor("#6b7280"));
        p.drawText(410, y + 2, QString("Score: %1").arg(e.score, 0, 'f', 0));

        p.setPen(e.color);
        p.drawText(490, y + 2, e.correct ? "Correct" : "Wrong");

        p.setFont(QFont("Sans", 10));
        p.setPen(QColor("#6b7280"));
        p.drawText(28, y + 18, QString("Category: %1 | %2").arg(e.category, e.answer));

        y += 52;
    }
}

void PaperQuizGenerator::drawCategoryChart(QPainter &p)
{
    if (entries_.isEmpty()) return;

    QMap<QString, int> counts;
    for (const auto &e : entries_) counts[e.category]++;

    int y = height() - 100;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("By Difficulty"));
    y += 18;

    const QStringList colors = {"#16a34a", "#245,158,11", "#239,68,68"};
    int idx = 0;
    int maxVal = *std::max_element(counts.constBegin(), counts.constEnd());
    if (maxVal == 0) maxVal = 1;

    p.setFont(QFont("Sans", 9));
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QColor barColor(colors.at(idx % colors.size()));
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        int bw = static_cast<int>((static_cast<double>(it.value()) / maxVal) * 140);
        p.drawRoundedRect(80, y - 10, bw, 16, 4, 4);

        p.setPen(QColor("#374151"));
        p.drawText(12, y + 2, it.key());
        p.drawText(80 + bw + 6, y + 2, QString::number(it.value()));
        y += 22;
        ++idx;
    }
}

void PaperQuizGenerator::drawStats(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int x = width() - 200;
    int y = height() - 90;
    const int correctCount = std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const QuizEntry &e) { return e.correct; });
    double avgScore = 0;
    for (const auto &e : entries_) avgScore += e.score;
    avgScore = (avgScore / entries_.size()) * 100.0;
    QSet<QString> cats;
    for (const auto &e : entries_) cats.insert(e.category);

    p.setFont(QFont("Sans", 9));
    p.setPen(QColor("#6b7280"));
    p.drawText(x, y, tr("Questions: %1").arg(entries_.size()));
    p.drawText(x, y + 16, tr("Correct: %1").arg(correctCount));
    p.drawText(x, y + 32, tr("Avg Score: %1%").arg(avgScore, 0, 'f', 1));
    p.drawText(x, y + 48, tr("Categories: %1").arg(cats.size()));
}

void PaperQuizGenerator::loadSettings()
{
    QSettings s("PaperCrawler", "PaperQuizGenerator");
    const int size = s.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        QuizEntry e;
        e.id = s.value("id").toInt();
        e.question = s.value("question").toString();
        e.category = s.value("category").toString();
        e.difficulty = s.value("difficulty").toString();
        e.answer = s.value("answer").toString();
        e.score = s.value("score").toDouble();
        e.correct = s.value("correct").toBool();
        e.color = QColor(s.value("color").toString());
        entries_.append(e);
        if (e.id >= nextId_) nextId_ = e.id + 1;
    }
    s.endArray();
    updateInfo();
    update();
}

void PaperQuizGenerator::saveSettings()
{
    QSettings s("PaperCrawler", "PaperQuizGenerator");
    s.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        s.setArrayIndex(i);
        const auto &e = entries_.at(i);
        s.setValue("id", e.id);
        s.setValue("question", e.question);
        s.setValue("category", e.category);
        s.setValue("difficulty", e.difficulty);
        s.setValue("answer", e.answer);
        s.setValue("score", e.score);
        s.setValue("correct", e.correct);
        s.setValue("color", e.color.name());
    }
    s.endArray();
}
