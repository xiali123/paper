#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RangeEntry {
    int id; QString label; QString category; QString group;
    qreal low; qreal high; qreal mid; qreal range; bool wide; QColor color;
};
class PaperRangeChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperRangeChart(QWidget* parent = nullptr);
    void addEntry(const RangeEntry& entry);
    QList<RangeEntry> entries() const;
    int wideCount() const;
    qreal maxRange() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rangeRendered(int id, qreal range);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRangeView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RangeEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
