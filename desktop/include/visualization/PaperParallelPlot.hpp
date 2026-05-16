#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ParallelEntry {
    int id; QString label; QString category; QString axis;
    qreal x1; qreal x2; qreal x3; bool outlier; QColor color;
};
class PaperParallelPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperParallelPlot(QWidget* parent = nullptr);
    void addEntry(const ParallelEntry& entry);
    QList<ParallelEntry> entries() const;
    int outlierCount() const;
    qreal avgRange() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void parallelRendered(int id, qreal range);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawParallelView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ParallelEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
