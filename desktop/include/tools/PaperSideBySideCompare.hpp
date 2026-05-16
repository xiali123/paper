#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct ComparisonEntry {
    int id{-1};
    QString metric;
    QString paperA;
    QString paperB;
    qreal valueA{0};
    qreal valueB{0};
    QString verdict; // "A wins", "B wins", "tie"
    QString category;
    QColor colorA;
    QColor colorB;
};

class PaperSideBySideCompare : public QWidget {
    Q_OBJECT

public:
    explicit PaperSideBySideCompare(QWidget* parent = nullptr);

    void addComparison(const ComparisonEntry& entry);
    QList<ComparisonEntry> comparisons() const;
    QMap<QString, int> verdictCounts() const;
    int aWins() const;
    int bWins() const;

signals:
    void comparisonComplete(int aWins, int bWins);

private slots:
    void onAdd();
    void onFilterChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawComparisonBars(QPainter& p, const QRect& rect);
    void drawVerdictChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ComparisonEntry> comparisons_;
    QSettings settings_;
};
