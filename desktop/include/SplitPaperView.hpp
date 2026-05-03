#pragma once

#include <QWidget>
#include <QSplitter>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QList>

struct Paper;

class SplitPaperView : public QWidget {
    Q_OBJECT

public:
    explicit SplitPaperView(QWidget* parent = nullptr);

    void setLeftPaper(const Paper& paper);
    void setRightPaper(const Paper& paper);
    void clear();

signals:
    void paperClicked(int paperId);
    void compareRequested(int leftId, int rightId);

private:
    void setupUI();
    QWidget* createPaperPanel(const QString& label);

    QSplitter* splitter_{nullptr};
    QLabel* leftTitle_{nullptr};
    QTextEdit* leftContent_{nullptr};
    QLabel* rightTitle_{nullptr};
    QTextEdit* rightContent_{nullptr};
    QPushButton* compareBtn_{nullptr};

    int leftPaperId_{0};
    int rightPaperId_{0};
};
