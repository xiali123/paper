#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Waterfall2Entry {
    int id; QString label; QString category; QString stage;
    qreal value; qreal cumulative; bool positive; QColor color;
};
class PaperWaterfallPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperWaterfallPlot2(QWidget* parent = nullptr);
    void addEntry(const Waterfall2Entry& entry);
    QList<Waterfall2Entry> entries() const;
    int positiveCount() const;
    qreal maxCumulative() const;
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
    void drawWaterfall(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Waterfall2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
