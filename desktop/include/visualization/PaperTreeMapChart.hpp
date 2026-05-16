#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TreeMapEntry {
    int id; QString label; QString category; QString parent;
    qreal size; qreal value; int depth; bool dominant; QColor color;
};
class PaperTreeMapChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperTreeMapChart(QWidget* parent = nullptr);
    void addEntry(const TreeMapEntry& entry);
    QList<TreeMapEntry> entries() const;
    int dominantCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rectSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTreeMapView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TreeMapEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
