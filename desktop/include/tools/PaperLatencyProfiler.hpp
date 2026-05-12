#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LatencyEntry {
    int id; QString endpoint; QString category; QString method;
    qreal latency; int calls; bool slow; QColor color;
};
class PaperLatencyProfiler : public QWidget {
    Q_OBJECT
public:
    explicit PaperLatencyProfiler(QWidget* parent = nullptr);
    void addEntry(const LatencyEntry& entry);
    QList<LatencyEntry> entries() const;
    int slowCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void profileComplete(int id, qreal latency);
private slots:
    void onProfile();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLatencyChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LatencyEntry> entries_;
    QSettings settings_;
    QPushButton* profileBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
