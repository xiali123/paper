#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QList>
#include <QSettings>

struct NotificationRule {
    int id{-1};
    QString name;
    QString event;
    QString condition;
    QString conditionValue;
    QString action;
    bool enabled{true};
};

class NotificationRuleEditor : public QWidget {
    Q_OBJECT

public:
    explicit NotificationRuleEditor(QWidget* parent = nullptr);

    void addRule(const NotificationRule& rule);
    void removeRule(int ruleId);
    void enableRule(int ruleId, bool enabled);
    QList<NotificationRule> rules() const;
    QList<NotificationRule> matchRules(const QString& event, const QString& data) const;

signals:
    void ruleCreated(const NotificationRule& rule);
    void ruleDeleted(int ruleId);
    void ruleTriggered(int ruleId, const QString& action);

private slots:
    void onAdd();
    void onRemove();
    void onToggle();
    void onTest();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshList();

    QListWidget* ruleList_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QComboBox* eventCombo_{nullptr};
    QComboBox* conditionCombo_{nullptr};
    QLineEdit* valueEdit_{nullptr};
    QComboBox* actionCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* toggleBtn_{nullptr};
    QPushButton* testBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<NotificationRule> rules_;
    int nextId_{1};
};
