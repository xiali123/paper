#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SwarmPlotEntry {
    int id; QString label; QString category; QString group;
    qreal x; qreal y; qreal value; bool outlier; QColor color;
};
class PaperSwarmPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperSwarmPlot(QWidget* parent = nullptr);
    void addEntry(const SwarmPlotEntry& entry);
    QList<SwarmPlotEntry> entries() const;
    int outlierCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pointSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSwarmView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SwarmPlotEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
