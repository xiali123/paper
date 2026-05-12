#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct MarimekkoEntry {
    int id; QString label; QString category; QString segment;
    qreal width; qreal height; qreal value; bool dominant; QColor color;
};
class PaperMarimekkoChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperMarimekkoChart2(QWidget* parent = nullptr);
    void addEntry(const MarimekkoEntry& entry);
    QList<MarimekkoEntry> entries() const;
    int dominantCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void segmentSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMarimekkoView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<MarimekkoEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
