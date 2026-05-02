#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>

class QPlainTextEdit;

class LatexFindReplaceBar : public QWidget {
    Q_OBJECT

public:
    explicit LatexFindReplaceBar(QPlainTextEdit* editor, QWidget* parent = nullptr);

    void activateFind();
    void activateReplace();

private slots:
    void onFindNext();
    void onFindPrev();
    void onReplace();
    void onReplaceAll();
    void onTextChanged(const QString& text);

private:
    void setupUI();
    void find(bool forward);
    void highlightAll();

    QPlainTextEdit* editor_{nullptr};

    QLineEdit* findEdit_{nullptr};
    QLineEdit* replaceEdit_{nullptr};
    QPushButton* findNextBtn_{nullptr};
    QPushButton* findPrevBtn_{nullptr};
    QPushButton* replaceBtn_{nullptr};
    QPushButton* replaceAllBtn_{nullptr};
    QCheckBox* caseCheck_{nullptr};
    QCheckBox* regexCheck_{nullptr};
    QCheckBox* wholeWordCheck_{nullptr};
    QLabel* resultLabel_{nullptr};

    int currentMatch_{-1};
    int totalMatches_{0};
};
