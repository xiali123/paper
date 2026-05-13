#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PingMonitorEntry {
    int id; QString host; QString category; QString protocol;
    qreal latency; int hops; bool reachable; QColor color;
};
class PaperPingMonitor : public QWidget {
    Q_OBJECT
public:
    explicit PaperPingMonitor(QWidget* parent = nullptr);
    void addEntry(const PingMonitorEntry& entry);
    QList<PingMonitorEntry> entries() const;
    int reachableCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void hostChecked(int id, qreal latency);
private slots:
    void onPing();
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
    QList<PingMonitorEntry> entries_;
    QSettings settings_;
    QPushButton* pingBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
