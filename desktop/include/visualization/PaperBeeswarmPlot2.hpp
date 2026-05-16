#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BeeswarmPlot2Entry {
    int id; QString series; QString category; QString group;
    qreal value; int jitter; bool extreme; QColor color;
};
class PaperBeeswarmPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperBeeswarmPlot2(QWidget* parent = nullptr);
    void addEntry(const BeeswarmPlot2Entry& entry);
    QList<BeeswarmPlot2Entry> entries() const;
    int extremeCount() const;
    qreal medianValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pointHovered(int id, qreal value);
private slots:
    void onPlot();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBeeswarm(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BeeswarmPlot2Entry> entries_;
    QSettings settings_;
    QPushButton* plotBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
