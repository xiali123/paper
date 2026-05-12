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
    qreal value; qreal max; bool filled; QColor color;
};
class PaperRadialBarChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperRadialBarChart(QWidget* parent = nullptr);
    void addEntry(const RadialEntry& entry);
    QList<RadialEntry> entries() const;
    int filledCount() const;
    qreal avgFill() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void ringSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRadialChart(QPainter& p, const QRect& rect);
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
