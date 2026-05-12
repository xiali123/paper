#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RidgelineEntry {
    int id; QString label; QString category; QString group;
    qreal value; qreal density; int rank; bool peak; QColor color;
};
class PaperRidgelinePlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperRidgelinePlot(QWidget* parent = nullptr);
    void addEntry(const RidgelineEntry& entry);
    QList<RidgelineEntry> entries() const;
    int peakCount() const;
    qreal maxDensity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void ridgeSelected(int id, qreal density);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRidgelines(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RidgelineEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
