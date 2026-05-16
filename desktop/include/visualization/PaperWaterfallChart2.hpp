#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WaterfallEntry {
    int id; QString label; QString category; QString type;
    qreal value; qreal cumulative; bool positive; QColor color;
};
class PaperWaterfallChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperWaterfallChart2(QWidget* parent = nullptr);
    void addEntry(const WaterfallEntry& entry);
    QList<WaterfallEntry> entries() const;
    int positiveCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void barSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawWaterfall(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WaterfallEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
