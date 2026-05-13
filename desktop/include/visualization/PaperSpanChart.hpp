#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SpanChartEntry {
    int id; QString task; QString category; QString phase;
    qreal duration; int segments; bool critical; QColor color;
};
class PaperSpanChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperSpanChart(QWidget* parent = nullptr);
    void addEntry(const SpanChartEntry& entry);
    QList<SpanChartEntry> entries() const;
    int criticalCount() const;
    qreal totalDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void spanSelected(int id, qreal duration);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSpanChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SpanChartEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
