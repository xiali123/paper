#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ViolinChart2Entry {
    int id; QString metric; QString category; QString group;
    qreal median; int samples; bool outlier; QColor color;
};
class PaperViolinChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperViolinChart2(QWidget* parent = nullptr);
    void addEntry(const ViolinChart2Entry& entry);
    QList<ViolinChart2Entry> entries() const;
    int outlierCount() const;
    qreal avgMedian() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void violinSelected(int id, qreal median);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawViolinChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ViolinChart2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
