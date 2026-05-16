#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct QueryOptimizer2Entry {
    int id; QString query; QString category; QString strategy;
    qreal speedup; int executions; bool optimal; QColor color;
};
class PaperQueryOptimizer2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperQueryOptimizer2(QWidget* parent = nullptr);
    void addEntry(const QueryOptimizer2Entry& entry);
    QList<QueryOptimizer2Entry> entries() const;
    int optimalCount() const;
    qreal avgSpeedup() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void queryOptimized(int id, qreal speedup);
private slots:
    void onOptimize();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawOptimizerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<QueryOptimizer2Entry> entries_;
    QSettings settings_;
    QPushButton* optimizeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
