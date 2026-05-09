#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct TransformRule {
    int id{-1};
    QString name;
    QString inputFormat;
    QString outputFormat;
    QString operation; // "convert", "normalize", "merge", "filter"
    int recordsProcessed{0};
    bool active{false};
    QColor color;
};

class PaperDataTransformWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperDataTransformWidget(QWidget* parent = nullptr);

    void addRule(const TransformRule& rule);
    QList<TransformRule> rules() const;
    QMap<QString, int> operationCounts() const;
    int activeRules() const;
    int totalProcessed() const;

signals:
    void ruleApplied(int ruleId);
    void transformComplete(int records);

private slots:
    void onAdd();
    void onRunAll();
    void onFilterChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawRuleList(QPainter& p, const QRect& rect);
    void drawOperationChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* runAllBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<TransformRule> rules_;
    QSettings settings_;
};
