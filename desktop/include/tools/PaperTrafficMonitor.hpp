#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TrafficEntry {
    int id; QString route; QString category; QString method;
    qreal rps; int errors; bool healthy; QColor color;
};
class PaperTrafficMonitor : public QWidget {
    Q_OBJECT
public:
    explicit PaperTrafficMonitor(QWidget* parent = nullptr);
    void addEntry(const TrafficEntry& entry);
    QList<TrafficEntry> entries() const;
    int healthyCount() const;
    qreal avgRps() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void trafficAlert(int id, qreal rps);
private slots:
    void onMonitor();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTrafficView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TrafficEntry> entries_;
    QSettings settings_;
    QPushButton* monitorBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
