#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StepEntry {
    int id; QString step; QString category; QString phase;
    qreal value; qreal baseline; qreal delta; bool improved; QColor color;
};
class PaperStepChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperStepChart(QWidget* parent = nullptr);
    void addEntry(const StepEntry& entry);
    QList<StepEntry> entries() const;
    int improvedCount() const;
    qreal maxDelta() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void stepRendered(int id, qreal delta);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStepView(QPainter& p, const QRect& rect);
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
