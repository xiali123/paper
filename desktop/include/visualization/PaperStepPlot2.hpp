#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StepPlot2Entry {
    int id; QString series; QString category; QString phase;
    qreal value; int step; bool ascending; QColor color;
};
class PaperStepPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperStepPlot2(QWidget* parent = nullptr);
    void addEntry(const StepPlot2Entry& entry);
    QList<StepPlot2Entry> entries() const;
    int ascendingCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void stepReached(int id, qreal value);
private slots:
    void onPlot();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStepPlot(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StepPlot2Entry> entries_;
    QSettings settings_;
    QPushButton* plotBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
