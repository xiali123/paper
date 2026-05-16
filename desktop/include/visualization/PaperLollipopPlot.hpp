#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LollipopEntry {
    int id; QString label; QString category; QString group;
    qreal value; qreal threshold; bool above; QColor color;
};
class PaperLollipopPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperLollipopPlot(QWidget* parent = nullptr);
    void addEntry(const LollipopEntry& entry);
    QList<LollipopEntry> entries() const;
    int aboveCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void stickSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLollipopChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LollipopEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
