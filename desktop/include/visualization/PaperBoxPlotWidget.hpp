#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BoxEntry {
    int id; QString label; QString category; QString group;
    qreal q1; qreal median; qreal q3; qreal whiskerLow; qreal whiskerHigh; bool outlier; QColor color;
};
class PaperBoxPlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaperBoxPlotWidget(QWidget* parent = nullptr);
    void addEntry(const BoxEntry& entry);
    QList<BoxEntry> entries() const;
    int outlierCount() const;
    qreal maxMedian() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void boxRendered(int id, qreal median);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBoxView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BoxEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
