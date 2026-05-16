#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StepEntry {
    int id; QString label; QString category; QString axis;
    qreal value; qreal step; bool rising; QColor color;
};
class PaperStepPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperStepPlot(QWidget* parent = nullptr);
    void addEntry(const StepEntry& entry);
    QList<StepEntry> entries() const;
    int risingCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void stepClicked(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStepChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StepEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
