#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct CostEntry {
    int id;
    QString itemName;
    QString category;
    qreal estimatedCost;
    qreal actualCost;
    QString currency;
    QString period;
    qreal variance;
    bool overBudget;
    QColor color;
};

class PaperCostEstimator : public QWidget {
    Q_OBJECT
public:
    explicit PaperCostEstimator(QWidget* parent = nullptr);
    void addEntry(const CostEntry& entry);
    QList<CostEntry> entries() const;
    qreal totalEstimated() const;
    int overBudgetCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void costEstimated(int id, qreal estimatedCost);

private slots:
    void onEstimate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCostList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<CostEntry> entries_;
    QPushButton* estimateBtn_;
    QPushButton* clearBtn_;
    QComboBox* periodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
