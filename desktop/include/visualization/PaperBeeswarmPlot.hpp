#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BeeswarmEntry {
    int id; QString label; QString category; QString group;
    qreal value; qreal jitter; bool outlier; QColor color;
};
class PaperBeeswarmPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperBeeswarmPlot(QWidget* parent = nullptr);
    void addEntry(const BeeswarmEntry& entry);
    QList<BeeswarmEntry> entries() const;
    int outlierCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dotSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBeeswarm(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BeeswarmEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
