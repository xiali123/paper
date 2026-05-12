#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct MekkoEntry {
    int id; QString segment; QString category; QString axis;
    qreal width; qreal height; qreal area; bool dominant; QColor color;
};
class PaperMarimekkoChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperMarimekkoChart(QWidget* parent = nullptr);
    void addEntry(const MekkoEntry& entry);
    QList<MekkoEntry> entries() const;
    int dominantCount() const;
    qreal totalArea() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void mekkRendered(int id, qreal area);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMekkoView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<MekkoEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
