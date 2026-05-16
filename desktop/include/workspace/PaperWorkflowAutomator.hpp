#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct WorkflowStep {
    int id{-1};
    QString name;
    QString action;
    QString params;
    bool enabled{true};
    int order{0};
};

struct WorkflowChain {
    int id{-1};
    QString name;
    QString trigger;
    QList<WorkflowStep> steps;
    bool active{true};
    qint64 createdAt{0};
};

class PaperWorkflowAutomator : public QWidget {
    Q_OBJECT

public:
    explicit PaperWorkflowAutomator(QWidget* parent = nullptr);

    void addChain(const WorkflowChain& chain);
    void removeChain(int chainId);
    QList<WorkflowChain> chains() const;
    void executeChain(int chainId);

signals:
    void chainExecuted(int chainId, const QString& name);
    void chainCreated(int chainId);
    void chainDeleted(int chainId);
    void stepCompleted(int chainId, int stepId);

private slots:
    void onCreateChain();
    void onDeleteChain();
    void onAddStep();
    void onRemoveStep();
    void onExecute();
    void onChainSelected();
    void onToggleActive();

private:
    void setupUI();
    void refreshChainList();
    void refreshStepList();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QListWidget* chainList_{nullptr};
    QListWidget* stepList_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QComboBox* triggerCombo_{nullptr};
    QLineEdit* stepNameEdit_{nullptr};
    QComboBox* actionCombo_{nullptr};
    QTextEdit* paramsEdit_{nullptr};
    QPushButton* createChainBtn_{nullptr};
    QPushButton* deleteChainBtn_{nullptr};
    QPushButton* addStepBtn_{nullptr};
    QPushButton* removeStepBtn_{nullptr};
    QPushButton* executeBtn_{nullptr};
    QPushButton* toggleBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<WorkflowChain> chains_;
    int nextChainId_{1};
    int nextStepId_{1};
    int selectedChainId_{-1};
};
