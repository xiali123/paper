#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct MetricEntry {
    int id; QString metric; QString category; QString format;
    qreal value; int points; bool exported; QColor color;
};
class PaperMetricExporter : public QWidget {
    Q_OBJECT
public:
    explicit PaperMetricExporter(QWidget* parent = nullptr);
    void addEntry(const MetricEntry& entry);
    QList<MetricEntry> entries() const;
    int exportedCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void metricExported(int id, qreal value);
private slots:
    void onExport();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMetricView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<MetricEntry> entries_;
    QSettings settings_;
    QPushButton* exportBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
