#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ProbeEntry {
    int id; QString service; QString category; QString endpoint;
    qreal latency; int checks; bool healthy; QColor color;
};
class PaperServiceProbe : public QWidget {
    Q_OBJECT
public:
    explicit PaperServiceProbe(QWidget* parent = nullptr);
    void addEntry(const ProbeEntry& entry);
    QList<ProbeEntry> entries() const;
    int healthyCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void probeComplete(int id, qreal latency);
private slots:
    void onProbe();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawProbeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ProbeEntry> entries_;
    QSettings settings_;
    QPushButton* probeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
