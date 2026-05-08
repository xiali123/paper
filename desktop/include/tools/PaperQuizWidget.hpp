#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QListWidget>
#include <QComboBox>
#include <QMap>
#include <QList>
#include <QSettings>

struct QuizCard {
    int id{-1};
    int paperId{-1};
    QString paperTitle;
    QString question;
    QString answer;
    QString category;
    int correctCount{0};
    int wrongCount{0};
};

class PaperQuizWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperQuizWidget(QWidget* parent = nullptr);

    void addCard(const QuizCard& card);
    void addCardsFromPaper(int paperId, const QString& title, const QString& abstract);
    QList<QuizCard> cards() const;
    int dueCardCount() const;
    void startQuiz();
    void startQuizByCategory(const QString& category);

signals:
    void cardAnswered(int cardId, bool correct);
    void quizCompleted(int total, int correct, int wrong);
    void cardAdded(const QuizCard& card);

private slots:
    void onShowAnswer();
    void onCorrect();
    void onWrong();
    void onAddCard();
    void onDeleteCard();
    void onCategoryChanged(int index);
    void onModeChanged(int index);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void showCurrentCard();
    void showResult();
    void refreshCardList();
    void refreshCategories();

    QLabel* cardCountLabel_{nullptr};
    QLabel* progressLabel_{nullptr};
    QTextEdit* questionEdit_{nullptr};
    QTextEdit* answerEdit_{nullptr};
    QPushButton* showAnswerBtn_{nullptr};
    QPushButton* correctBtn_{nullptr};
    QPushButton* wrongBtn_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* startBtn_{nullptr};
    QComboBox* categoryCombo_{nullptr};
    QComboBox* modeCombo_{nullptr};
    QListWidget* cardList_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<QuizCard> allCards_;
    QList<QuizCard> quizDeck_;
    int currentCardIndex_{-1};
    bool answerRevealed_{false};
    int quizCorrect_{0};
    int quizWrong_{0};
    int nextCardId_{1};
};
