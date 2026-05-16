#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct BudgetEntry {
    int id;
    QString item;
    QString category;
    QString type;
    qreal allocated;
    qreal spent;
    qreal remaining;
    bool overBudget;
    QColor color;
};

class PaperBudgetTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperBudgetTracker(QWidget* parent = nullptr);
    void addEntry(const BudgetEntry& entry);
    QList<BudgetEntry> entries() const;
    int overBudgetCount() const;
    qreal totalSpent() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void budgetUpdated(int id, qreal remaining);

private slots:
    void onAdd();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawBudgetList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<BudgetEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
