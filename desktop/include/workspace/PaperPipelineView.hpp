#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PipeEntry {
    int id; QString stage; QString category; QString status;
    qreal latency; qreal throughput; QString connector; bool bottleneck; QColor color;
};
class PaperPipelineView : public QWidget {
    Q_OBJECT
public:
    explicit PaperPipelineView(QWidget* parent = nullptr);
    void addEntry(const PipeEntry& entry);
    QList<PipeEntry> entries() const;
    int bottleneckCount() const;
    qreal totalLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pipelineUpdated(int id, qreal latency);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPipelineView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PipeEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
