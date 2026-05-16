#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct ValidationRule {
    int id{-1};
    QString name;
    QString field; // "title", "authors", "abstract", "references", "data"
    QString type; // "required", "format", "range", "custom"
    QString status; // "pass", "warning", "fail"
    int checksRun{0};
    int checksPassed{0};
    QString message;
    QColor color;
};

class PaperValidationWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperValidationWidget(QWidget* parent = nullptr);

    void addRule(const ValidationRule& rule);
    QList<ValidationRule> rules() const;
    QMap<QString, int> statusCounts() const;
    qreal passRate() const;
    int totalChecks() const;

signals:
    void validationComplete(int passed, int total);
    void ruleViolated(int ruleId, const QString& status);

private slots:
    void onAdd();
    void onValidate();
    void onFilterChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawRuleList(QPainter& p, const QRect& rect);
    void drawResultChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* validateBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ValidationRule> rules_;
    QSettings settings_;
};
