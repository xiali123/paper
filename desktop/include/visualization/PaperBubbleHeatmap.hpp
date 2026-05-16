#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BubbleHeatEntry {
    int id; QString label; QString category; QString row;
    qreal x; qreal y; qreal size; bool hotspot; QColor color;
};
class PaperBubbleHeatmap : public QWidget {
    Q_OBJECT
public:
    explicit PaperBubbleHeatmap(QWidget* parent = nullptr);
    void addEntry(const BubbleHeatEntry& entry);
    QList<BubbleHeatEntry> entries() const;
    int hotspotCount() const;
    qreal maxSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bubbleClicked(int id, qreal size);
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
    QList<BubbleHeatEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
