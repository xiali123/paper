#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Waterfall3Entry {
    int id; QString phase; QString category; QString metric;
    qreal duration; int steps; bool critical; QColor color;
};
class PaperWaterfall3 : public QWidget {
    Q_OBJECT
public:
    explicit PaperWaterfall3(QWidget* parent = nullptr);
    void addEntry(const Waterfall3Entry& entry);
    QList<Waterfall3Entry> entries() const;
    int criticalCount() const;
    qreal totalDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void phaseSelected(int id, qreal duration);
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
    QList<Waterfall3Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
