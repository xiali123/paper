#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DumbbellPlot2Entry {
    int id; QString pair; QString category; QString metric;
    qreal leftVal; qreal rightVal; bool improved; QColor color;
};
class PaperDumbbellPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperDumbbellPlot2(QWidget* parent = nullptr);
    void addEntry(const DumbbellPlot2Entry& entry);
    QList<DumbbellPlot2Entry> entries() const;
    int improvedCount() const;
    qreal avgChange() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pairSelected(int id, qreal change);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDumbbellPlot(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DumbbellPlot2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
