#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DumbbellEntry {
    int id; QString label; QString category; QString group;
    qreal left; qreal right; qreal gap; bool significant; QColor color;
};
class PaperDumbbellPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperDumbbellPlot(QWidget* parent = nullptr);
    void addEntry(const DumbbellEntry& entry);
    QList<DumbbellEntry> entries() const;
    int significantCount() const;
    qreal maxGap() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dumbbellSelected(int id, qreal gap);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDumbbellChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DumbbellEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
