#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct QuizEntry {
    int id;
    QString question;
    QString category;
    QString difficulty;
    QString answer;
    qreal score;
    bool correct;
    QColor color;
};

class PaperQuizGenerator : public QWidget {
    Q_OBJECT
public:
    explicit PaperQuizGenerator(QWidget* parent = nullptr);
    void addEntry(const QuizEntry& entry);
    QList<QuizEntry> entries() const;
    int correctCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void quizGenerated(int id, qreal score);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawQuizList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<QuizEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
