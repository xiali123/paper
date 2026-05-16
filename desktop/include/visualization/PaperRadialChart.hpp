#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RadialEntry {
    int id; QString label; QString category; QString ring;
    qreal value; qreal angle; qreal radius; bool highlighted; QColor color;
};
class PaperRadialChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperRadialChart(QWidget* parent = nullptr);
    void addEntry(const RadialEntry& entry);
    QList<RadialEntry> entries() const;
    int highlightedCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void radialRendered(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRadialView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RadialEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
