#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ContourPlot2Entry {
    int id; QString variable; QString category; QString level;
    qreal value; int contours; bool peak; QColor color;
};
class PaperContourPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperContourPlot2(QWidget* parent = nullptr);
    void addEntry(const ContourPlot2Entry& entry);
    QList<ContourPlot2Entry> entries() const;
    int peakCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void contourSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawContourPlot(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ContourPlot2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
