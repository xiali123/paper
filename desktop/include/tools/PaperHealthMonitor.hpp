#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct HealthEntry {
    int id;
    QString service;
    QString status;
    int uptime;
    qreal responseTime;
    QString region;
    int errorCount;
    int requestCount;
    qreal errorRate;
    QString lastCheck;
    bool healthy;
    QColor color;
};

class PaperHealthMonitor : public QWidget {
    Q_OBJECT
public:
    explicit PaperHealthMonitor(QWidget* parent = nullptr);
    void addEntry(const HealthEntry& entry);
    QList<HealthEntry> entries() const;
    qreal avgResponseTime() const;
    int healthyCount() const;
    QMap<QString, int> regionCounts() const;

signals:
    void healthChecked(int id, qreal responseTime);

private slots:
    void onCheck();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawHealthList(QPainter& p, const QRect& rect);
    void drawRegionChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<HealthEntry> entries_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* regionCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
