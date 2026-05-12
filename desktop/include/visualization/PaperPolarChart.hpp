#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PolarEntry {
    int id; QString label; QString category; QString group;
    qreal angle; qreal radius; qreal value; bool dominant; QColor color;
};
class PaperPolarChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperPolarChart(QWidget* parent = nullptr);
    void addEntry(const PolarEntry& entry);
    QList<PolarEntry> entries() const;
    int dominantCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void polarRendered(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPolarView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PolarEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
