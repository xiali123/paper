#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BubbleHeatmap2Entry {
    int id; QString cell; QString category; QString axis;
    qreal intensity; int density; bool hotspot; QColor color;
};
class PaperBubbleHeatmap2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperBubbleHeatmap2(QWidget* parent = nullptr);
    void addEntry(const BubbleHeatmap2Entry& entry);
    QList<BubbleHeatmap2Entry> entries() const;
    int hotspotCount() const;
    qreal avgIntensity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void cellSelected(int id, qreal intensity);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHeatmap(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BubbleHeatmap2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
