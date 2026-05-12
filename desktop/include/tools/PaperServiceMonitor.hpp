#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ServiceEntry {
    int id; QString name; QString category; QString status;
    qreal uptime; qreal latency; int errors; bool healthy; QColor color;
};
class PaperServiceMonitor : public QWidget {
    Q_OBJECT
public:
    explicit PaperServiceMonitor(QWidget* parent = nullptr);
    void addEntry(const ServiceEntry& entry);
    QList<ServiceEntry> entries() const;
    int healthyCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void serviceChecked(int id, qreal latency);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawServiceList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ServiceEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
