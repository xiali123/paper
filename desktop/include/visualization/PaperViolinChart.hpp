#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ViolinEntry {
    int id; QString label; QString category; QString group;
    qreal min; qreal q1; qreal median; qreal q3; qreal max; bool skewed; QColor color;
};
class PaperViolinChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperViolinChart(QWidget* parent = nullptr);
    void addEntry(const ViolinEntry& entry);
    QList<ViolinEntry> entries() const;
    int skewedCount() const;
    qreal maxMedian() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void violinRendered(int id, qreal median);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawViolinView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ViolinEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
