#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HexbinEntry {
    int id; QString label; QString category; QString axis;
    qreal x; qreal y; int density; bool outlier; QColor color;
};
class PaperHexbinChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperHexbinChart(QWidget* parent = nullptr);
    void addEntry(const HexbinEntry& entry);
    QList<HexbinEntry> entries() const;
    int outlierCount() const;
    qreal avgDensity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void hexClicked(int id, qreal x, qreal y);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHexbin(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HexbinEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
