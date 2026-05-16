#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct SummaryStep {
    int id{-1};
    QString name;
    QString prompt;
    QString output;
    bool enabled{true};
};

struct SummaryChain {
    int id{-1};
    QString name;
    QList<SummaryStep> steps;
    QString input;
    QString finalOutput;
    qint64 createdAt{0};
};

class PaperSummarizerChain : public QWidget {
    Q_OBJECT

public:
    explicit PaperSummarizerChain(QWidget* parent = nullptr);

    void addChain(const SummaryChain& chain);
    void removeChain(int chainId);
    QList<SummaryChain> chains() const;
    void runChain(int chainId, const QString& input);

signals:
    void chainCreated(int chainId);
    void chainCompleted(int chainId, const QString& output);
    void stepCompleted(int chainId, int stepId, const QString& output);

private slots:
    void onCreateChain();
    void onDeleteChain();
    void onAddStep();
    void onRemoveStep();
    void onRun();
    void onChainSelected();
    void onStepSelected();

private:
    void setupUI();
    void refreshChainList();
    void refreshStepList();
    void updateStats();
    void loadSettings();
    void saveSettings();
    QString processStep(const QString& input, const QString& prompt);

    QListWidget* chainList_{nullptr};
    QListWidget* stepList_{nullptr};
    QTextEdit* inputEdit_{nullptr};
    QTextEdit* outputEdit_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QLineEdit* stepNameEdit_{nullptr};
    QTextEdit* promptEdit_{nullptr};
    QPushButton* createBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* addStepBtn_{nullptr};
    QPushButton* removeStepBtn_{nullptr};
    QPushButton* runBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<SummaryChain> chains_;
    int nextChainId_{1};
    int nextStepId_{1};
    int selectedChainId_{-1};
};
