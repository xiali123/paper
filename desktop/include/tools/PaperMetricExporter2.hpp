#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct MetricExporter2Entry {
    int id; QString metric; QString category; QString format;
    qreal volume; int exports; bool active; QColor color;
};
class PaperMetricExporter2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperMetricExporter2(QWidget* parent = nullptr);
    void addEntry(const MetricExporter2Entry& entry);
    QList<MetricExporter2Entry> entries() const;
    int activeCount() const;
    qreal avgVolume() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void exportComplete(int id, qreal volume);
private slots:
    void onExport();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawExporterView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<MetricExporter2Entry> entries_;
    QSettings settings_;
    QPushButton* exportBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
