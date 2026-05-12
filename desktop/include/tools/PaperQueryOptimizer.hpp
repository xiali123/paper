#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct QueryOptEntry {
    int id; QString query; QString category; QString plan;
    qreal speedup; int beforeMs; bool optimized; QColor color;
};
class PaperQueryOptimizer : public QWidget {
    Q_OBJECT
public:
    explicit PaperQueryOptimizer(QWidget* parent = nullptr);
    void addEntry(const QueryOptEntry& entry);
    QList<QueryOptEntry> entries() const;
    int optimizedCount() const;
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
    void drawQueryView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<QueryOptEntry> entries_;
    QSettings settings_;
    QPushButton* optimizeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
