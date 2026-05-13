#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TracerouteEntry {
    int id; QString destination; QString category; QString path;
    qreal latency; int hops; bool complete; QColor color;
};
class PaperTraceroute : public QWidget {
    Q_OBJECT
public:
    explicit PaperTraceroute(QWidget* parent = nullptr);
    void addEntry(const TracerouteEntry& entry);
    QList<TracerouteEntry> entries() const;
    int completeCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void routeTraced(int id, qreal latency);
private slots:
    void onTrace();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTraceView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TracerouteEntry> entries_;
    QSettings settings_;
    QPushButton* traceBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
