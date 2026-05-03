#include "PaperQuizWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>

PaperQuizWidget::PaperQuizWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperQuizWidget::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: card list
    auto* leftPanel = new QVBoxLayout();

    auto* topRow = new QHBoxLayout();
    modeCombo_ = new QComboBox();
    modeCombo_->addItems({"All Cards", "Due Cards", "Hardest", "Random 10"});
    connect(modeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperQuizWidget::onModeChanged);
    topRow->addWidget(modeCombo_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItem("All Categories");
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperQuizWidget::onCategoryChanged);
    topRow->addWidget(categoryCombo_, 1);
    leftPanel->addLayout(topRow);

    cardList_ = new QListWidget();
    cardList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 5px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    leftPanel->addWidget(cardList_, 1);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Card");
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
    );
    connect(addBtn_, &QPushButton::clicked, this, &PaperQuizWidget::onAddCard);
    btnRow->addWidget(addBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperQuizWidget::onDeleteCard);
    btnRow->addWidget(deleteBtn_);

    leftPanel->addLayout(btnRow);

    statsLabel_ = new QLabel("0 cards");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    leftPanel->addWidget(statsLabel_);

    auto* leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    splitter->addWidget(leftWidget);

    // Right: quiz area
    auto* rightPanel = new QVBoxLayout();

    auto* headerRow = new QHBoxLayout();
    cardCountLabel_ = new QLabel("No quiz active");
    cardCountLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    headerRow->addWidget(cardCountLabel_, 1);

    startBtn_ = new QPushButton("Start Quiz");
    startBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(startBtn_, &QPushButton::clicked, this, [this]() { startQuiz(); });
    headerRow->addWidget(startBtn_);
    rightPanel->addLayout(headerRow);

    progressLabel_ = new QLabel("");
    progressLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    rightPanel->addWidget(progressLabel_);

    rightPanel->addWidget(new QLabel("Question:"));
    questionEdit_ = new QTextEdit();
    questionEdit_->setReadOnly(true);
    questionEdit_->setMaximumHeight(120);
    questionEdit_->setStyleSheet("QTextEdit { font-size: 14px; padding: 8px; }");
    rightPanel->addWidget(questionEdit_);

    showAnswerBtn_ = new QPushButton("Show Answer");
    showAnswerBtn_->setStyleSheet(
        "QPushButton { background: #8b5cf6; color: white; padding: 8px 20px; "
        "border-radius: 6px; font-weight: bold; font-size: 13px; }"
    );
    connect(showAnswerBtn_, &QPushButton::clicked, this, &PaperQuizWidget::onShowAnswer);
    rightPanel->addWidget(showAnswerBtn_);

    rightPanel->addWidget(new QLabel("Answer:"));
    answerEdit_ = new QTextEdit();
    answerEdit_->setReadOnly(true);
    answerEdit_->setMaximumHeight(120);
    answerEdit_->setVisible(false);
    answerEdit_->setStyleSheet("QTextEdit { font-size: 14px; padding: 8px; background: #f0fdf4; }");
    rightPanel->addWidget(answerEdit);

    auto* answerRow = new QHBoxLayout();
    correctBtn_ = new QPushButton("Correct");
    correctBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(correctBtn_, &QPushButton::clicked, this, &PaperQuizWidget::onCorrect);
    correctBtn_->setVisible(false);
    answerRow->addWidget(correctBtn_);

    wrongBtn_ = new QPushButton("Wrong");
    wrongBtn_->setStyleSheet(
        "QPushButton { background: #dc2626; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(wrongBtn_, &QPushButton::clicked, this, &PaperQuizWidget::onWrong);
    wrongBtn_->setVisible(false);
    answerRow->addWidget(wrongBtn_);

    answerRow->addStretch();
    rightPanel->addLayout(answerRow);
    rightPanel->addStretch();

    auto* rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter);
}

void PaperQuizWidget::addCard(const QuizCard& card) {
    QuizCard c = card;
    if (c.id < 0) c.id = nextCardId_++;
    allCards_.append(c);
    nextCardId_ = qMax(nextCardId_, c.id + 1);
    saveSettings();
    refreshCardList();
    refreshCategories();
    emit cardAdded(c);
}

void PaperQuizWidget::addCardsFromPaper(int paperId, const QString& title, const QString& abstract) {
    QStringList sentences = abstract.split(QRegularExpression("[.!?]"), Qt::SkipEmptyParts);
    for (const auto& sentence : sentences) {
        QString trimmed = sentence.trimmed();
        if (trimmed.length() < 20) continue;

        QuizCard card;
        card.paperId = paperId;
        card.paperTitle = title;
        card.question = "Complete or explain:\n" + trimmed.left(trimmed.length() / 2) + "...";
        card.answer = trimmed;
        card.category = title.length() > 20 ? title.left(17) + "..." : title;
        card.id = nextCardId_++;
        allCards_.append(card);
    }
    saveSettings();
    refreshCardList();
    refreshCategories();
}

QList<QuizCard> PaperQuizWidget::cards() const { return allCards_; }

int PaperQuizWidget::dueCardCount() const {
    int count = 0;
    for (const auto& c : allCards_) {
        if (c.correctCount <= c.wrongCount) count++;
    }
    return count;
}

void PaperQuizWidget::startQuiz() {
    quizDeck_ = allCards_;
    QString cat = categoryCombo_->currentText();
    if (cat != "All Categories") {
        quizDeck_.erase(
            std::remove_if(quizDeck_.begin(), quizDeck_.end(),
                [&cat](const QuizCard& c) { return c.category != cat; }),
            quizDeck_.end());
    }

    int mode = modeCombo_->currentIndex();
    if (mode == 1) {
        quizDeck_.erase(
            std::remove_if(quizDeck_.begin(), quizDeck_.end(),
                [](const QuizCard& c) { return c.correctCount > c.wrongCount; }),
            quizDeck_.end());
    } else if (mode == 2) {
        std::sort(quizDeck_.begin(), quizDeck_.end(),
            [](const QuizCard& a, const QuizCard& b) {
                return (a.wrongCount - a.correctCount) > (b.wrongCount - b.correctCount);
            });
    } else if (mode == 3) {
        std::shuffle(quizDeck_.begin(), quizDeck_.end(), *QRandomGenerator::global());
        if (quizDeck_.size() > 10) quizDeck_ = quizDeck_.mid(0, 10);
    }

    if (quizDeck_.isEmpty()) return;

    currentCardIndex_ = 0;
    quizCorrect_ = 0;
    quizWrong_ = 0;
    showCurrentCard();
}

void PaperQuizWidget::startQuizByCategory(const QString& category) {
    int idx = categoryCombo_->findText(category);
    if (idx >= 0) categoryCombo_->setCurrentIndex(idx);
    startQuiz();
}

void PaperQuizWidget::onShowAnswer() {
    if (currentCardIndex_ < 0 || currentCardIndex_ >= quizDeck_.size()) return;
    answerRevealed_ = true;
    answerEdit_->setVisible(true);
    answerEdit_->setPlainText(quizDeck_[currentCardIndex_].answer);
    showAnswerBtn_->setVisible(false);
    correctBtn_->setVisible(true);
    wrongBtn_->setVisible(true);
}

void PaperQuizWidget::onCorrect() {
    if (currentCardIndex_ < 0 || currentCardIndex_ >= quizDeck_.size()) return;
    auto& card = quizDeck_[currentCardIndex_];
    card.correctCount++;
    for (auto& c : allCards_) {
        if (c.id == card.id) { c.correctCount++; break; }
    }
    quizCorrect_++;
    emit cardAnswered(card.id, true);
    advanceCard();
}

void PaperQuizWidget::onWrong() {
    if (currentCardIndex_ < 0 || currentCardIndex_ >= quizDeck_.size()) return;
    auto& card = quizDeck_[currentCardIndex_];
    card.wrongCount++;
    for (auto& c : allCards_) {
        if (c.id == card.id) { c.wrongCount++; break; }
    }
    quizWrong_++;
    emit cardAnswered(card.id, false);
    advanceCard();
}

void PaperQuizWidget::onAddCard() {
    QString q = QInputDialog::getMultiLineText(this, "Question", "Question:");
    if (q.trimmed().isEmpty()) return;
    QString a = QInputDialog::getMultiLineText(this, "Answer", "Answer:");
    if (a.trimmed().isEmpty()) return;
    QString cat = QInputDialog::getText(this, "Category", "Category:", QLineEdit::Normal, "General");
    QuizCard card;
    card.question = q;
    card.answer = a;
    card.category = cat;
    addCard(card);
}

void PaperQuizWidget::onDeleteCard() {
    int row = cardList_->currentRow();
    if (row < 0 || row >= allCards_.size()) return;
    allCards_.removeAt(row);
    saveSettings();
    refreshCardList();
}

void PaperQuizWidget::onCategoryChanged(int) { refreshCardList(); }
void PaperQuizWidget::onModeChanged(int) { refreshCardList(); }

void PaperQuizWidget::showCurrentCard() {
    if (currentCardIndex_ < 0 || currentCardIndex_ >= quizDeck_.size()) {
        showResult();
        return;
    }
    const auto& card = quizDeck_[currentCardIndex_];
    questionEdit_->setPlainText(card.question);
    answerEdit_->clear();
    answerEdit_->setVisible(false);
    showAnswerBtn_->setVisible(true);
    correctBtn_->setVisible(false);
    wrongBtn_->setVisible(false);
    answerRevealed_ = false;

    cardCountLabel_->setText(QString("Card %1 / %2").arg(currentCardIndex_ + 1).arg(quizDeck_.size()));
    progressLabel_->setText(QString("Correct: %1 | Wrong: %2").arg(quizCorrect_).arg(quizWrong_));
}

void PaperQuizWidget::showResult() {
    questionEdit_->setPlainText(
        QString("Quiz Complete!\n\nCorrect: %1\nWrong: %2\nAccuracy: %3%")
            .arg(quizCorrect_).arg(quizWrong_)
            .arg((quizCorrect_ + quizWrong_) > 0
                 ? quizCorrect_ * 100.0 / (quizCorrect_ + quizWrong_) : 0, 0, 'f', 0));
    answerEdit_->setVisible(false);
    showAnswerBtn_->setVisible(false);
    correctBtn_->setVisible(false);
    wrongBtn_->setVisible(false);
    cardCountLabel_->setText("Quiz finished");
    saveSettings();
    emit quizCompleted(quizCorrect_ + quizWrong_, quizCorrect_, quizWrong_);
}

void PaperQuizWidget::advanceCard() {
    currentCardIndex_++;
    if (currentCardIndex_ >= quizDeck_.size()) {
        showResult();
    } else {
        showCurrentCard();
    }
    saveSettings();
    refreshCardList();
}

void PaperQuizWidget::refreshCardList() {
    cardList_->clear();
    QString cat = categoryCombo_->currentText();
    for (const auto& card : allCards_) {
        if (cat != "All Categories" && card.category != cat) continue;
        QString display = QString("%1 | %2✓ %3✗")
            .arg(card.question.left(40)).arg(card.correctCount).arg(card.wrongCount);
        auto* item = new QListWidgetItem(display);
        if (card.correctCount > card.wrongCount)
            item->setForeground(QColor(5, 150, 105));
        else if (card.wrongCount > card.correctCount)
            item->setForeground(QColor(220, 38, 38));
        cardList_->addItem(item);
    }
    statsLabel_->setText(QString("%1 cards | %2 due").arg(allCards_.size()).arg(dueCardCount()));
}

void PaperQuizWidget::refreshCategories() {
    QString current = categoryCombo_->currentText();
    QSet<QString> cats;
    for (const auto& card : allCards_) cats.insert(card.category);
    categoryCombo_->blockSignals(true);
    categoryCombo_->clear();
    categoryCombo_->addItem("All Categories");
    for (const auto& cat : cats) categoryCombo_->addItem(cat);
    int idx = categoryCombo_->findText(current);
    if (idx >= 0) categoryCombo_->setCurrentIndex(idx);
    categoryCombo_->blockSignals(false);
}

void PaperQuizWidget::loadSettings() {
    QSettings settings("PaperCrawler", "PaperQuiz");
    QByteArray data = settings.value("cards").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        QuizCard card;
        card.id = obj["id"].toInt();
        card.paperId = obj["paperId"].toInt();
        card.paperTitle = obj["paperTitle"].toString();
        card.question = obj["question"].toString();
        card.answer = obj["answer"].toString();
        card.category = obj["category"].toString();
        card.correctCount = obj["correctCount"].toInt();
        card.wrongCount = obj["wrongCount"].toInt();
        allCards_.append(card);
        nextCardId_ = qMax(nextCardId_, card.id + 1);
    }
    refreshCardList();
    refreshCategories();
}

void PaperQuizWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& card : allCards_) {
        QJsonObject obj;
        obj["id"] = card.id;
        obj["paperId"] = card.paperId;
        obj["paperTitle"] = card.paperTitle;
        obj["question"] = card.question;
        obj["answer"] = card.answer;
        obj["category"] = card.category;
        obj["correctCount"] = card.correctCount;
        obj["wrongCount"] = card.wrongCount;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "PaperQuiz");
    settings.setValue("cards", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
