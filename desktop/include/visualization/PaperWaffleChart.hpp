#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WaffleEntry {
    int id; QString label; QString category; QString group;
    int count; qreal percentage; int cells; bool dominant; QColor color;
};
class PaperWaffleChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperWaffleChart(QWidget* parent = nullptr);
    void addEntry(const WaffleEntry& entry);
    QList<WaffleEntry> entries() const;
    int dominantCount() const;
    qreal maxPercentage() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void waffleRendered(int id, qreal percentage);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawWaffleView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WaffleEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
