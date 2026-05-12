#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EndpointMonitorEntry {
    int id; QString endpoint; QString category; QString status;
    qreal uptime; int errors; bool healthy; QColor color;
};
class PaperEndpointMonitor : public QWidget {
    Q_OBJECT
public:
    explicit PaperEndpointMonitor(QWidget* parent = nullptr);
    void addEntry(const EndpointMonitorEntry& entry);
    QList<EndpointMonitorEntry> entries() const;
    int healthyCount() const;
    qreal avgUptime() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void healthChecked(int id, qreal uptime);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawEndpointList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EndpointMonitorEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
