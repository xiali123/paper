#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

struct BudgetEntry {
    int id;
    QString item;
    QString category;
    qreal amount;
    qreal spent;
    QString status;
    QString deadline;
    QString notes;
    QColor color;
};

class PaperBudgetPlanner : public QWidget {
    Q_OBJECT
public:
    explicit PaperBudgetPlanner(QWidget* parent = nullptr);
    void addEntry(const BudgetEntry& entry);
    QList<BudgetEntry> entries() const;
    QMap<QString, int> categoryCounts() const;
    qreal totalBudget() const;
    qreal totalSpent() const;

signals:
    void budgetUpdated(qreal total);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawBudgetList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<BudgetEntry> entries_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
