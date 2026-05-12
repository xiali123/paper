#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RadialBarChart2Entry {
    int id; QString metric; QString category; QString group;
    qreal value; int rank; bool peak; QColor color;
};
class PaperRadialBarChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperRadialBarChart2(QWidget* parent = nullptr);
    void addEntry(const RadialBarChart2Entry& entry);
    QList<RadialBarChart2Entry> entries() const;
    int peakCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void barSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRadialBars(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RadialBarChart2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
