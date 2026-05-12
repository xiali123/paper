#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HeatEntry {
    int id; QString row; QString category; QString col;
    qreal value; qreal normalized; int rank; bool hot; QColor color;
};
class PaperHeatmapMatrix : public QWidget {
    Q_OBJECT
public:
    explicit PaperHeatmapMatrix(QWidget* parent = nullptr);
    void addEntry(const HeatEntry& entry);
    QList<HeatEntry> entries() const;
    int hotCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void heatmapRendered(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHeatmapView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HeatEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
