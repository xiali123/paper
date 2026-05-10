#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct BudgetEntry {
    int id;
    QString category;
    qreal allocated;
    qreal spent;
    qreal forecast;
    QString period;
    qreal variance;
    QString project;
    int papersAffected;
    bool onBudget;
    QColor color;
};

class PaperBudgetForecast : public QWidget {
    Q_OBJECT
public:
    explicit PaperBudgetForecast(QWidget* parent = nullptr);
    void addEntry(const BudgetEntry& entry);
    QList<BudgetEntry> entries() const;
    qreal totalAllocated() const;
    int onBudgetCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void forecastUpdated(int id, qreal variance);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onForecast();
    void onClear();
    void drawBudgetList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* categoryCombo_;
    QPushButton* forecastBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<BudgetEntry> entries_;
    QSettings settings_;
};
