#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PortRadarEntry {
    int id; QString port; QString category; QString service;
    qreal latency; int connections; bool open; QColor color;
};
class PaperPortRadar : public QWidget {
    Q_OBJECT
public:
    explicit PaperPortRadar(QWidget* parent = nullptr);
    void addEntry(const PortRadarEntry& entry);
    QList<PortRadarEntry> entries() const;
    int openCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void portScanned(int id, qreal latency);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRadarView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PortRadarEntry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
