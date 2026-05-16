#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TrafficMonitor2Entry {
    int id; QString route; QString category; QString method;
    qreal rps; int errors; bool overloaded; QColor color;
};
class PaperTrafficMonitor2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperTrafficMonitor2(QWidget* parent = nullptr);
    void addEntry(const TrafficMonitor2Entry& entry);
    QList<TrafficMonitor2Entry> entries() const;
    int overloadedCount() const;
    qreal avgRps() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void spikeDetected(int id, qreal rps);
private slots:
    void onMonitor();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMonitorView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TrafficMonitor2Entry> entries_;
    QSettings settings_;
    QPushButton* monitorBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
