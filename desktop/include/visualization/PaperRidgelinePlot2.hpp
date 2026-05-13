#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RidgelinePlot2Entry {
    int id; QString series; QString category; QString metric;
    qreal amplitude; int points; bool peak; QColor color;
};
class PaperRidgelinePlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperRidgelinePlot2(QWidget* parent = nullptr);
    void addEntry(const RidgelinePlot2Entry& entry);
    QList<RidgelinePlot2Entry> entries() const;
    int peakCount() const;
    qreal avgAmplitude() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void ridgeSelected(int id, qreal amplitude);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRidgeline(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RidgelinePlot2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
