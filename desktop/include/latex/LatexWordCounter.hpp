#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>

class QPlainTextEdit;

class LatexWordCounter : public QWidget {
    Q_OBJECT

public:
    explicit LatexWordCounter(QWidget* parent = nullptr);

    void updateCount(const QString& latexContent);
    void setDarkMode(bool dark);

private:
    void setupUI();

    QLabel* wordsLabel_{nullptr};
    QLabel* charsLabel_{nullptr};
    QLabel* charsNoCmdLabel_{nullptr};
    QLabel* linesLabel_{nullptr};
    QLabel* sectionsLabel_{nullptr};
    QLabel* equationsLabel_{nullptr};
    QLabel* environmentsLabel_{nullptr};
    QLabel* bibliographyLabel_{nullptr};

    bool darkMode_{false};
};
