#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Area2Entry {
    int id; QString label; QString category; QString series;
    qreal value; qreal baseline; bool stacked; QColor color;
};
class PaperAreaChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperAreaChart2(QWidget* parent = nullptr);
    void addEntry(const Area2Entry& entry);
    QList<Area2Entry> entries() const;
    int stackedCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void areaSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawAreaChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Area2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
