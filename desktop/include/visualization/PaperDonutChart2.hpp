#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DonutEntry {
    int id; QString label; QString category; QString segment;
    qreal value; qreal percentage; int rank; bool largest; QColor color;
};
class PaperDonutChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperDonutChart2(QWidget* parent = nullptr);
    void addEntry(const DonutEntry& entry);
    QList<DonutEntry> entries() const;
    int largestCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void segmentSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDonutView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DonutEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
