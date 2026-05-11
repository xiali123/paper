#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct AnomalyEntry {
    int id; QString metric; QString category; QString severity;
    qreal value; qreal threshold; qreal deviation; bool critical; QColor color;
};
class PaperAnomalyDetector : public QWidget {
    Q_OBJECT
public:
    explicit PaperAnomalyDetector(QWidget* parent = nullptr);
    void addEntry(const AnomalyEntry& entry);
    QList<AnomalyEntry> entries() const;
    int criticalCount() const;
    qreal maxDeviation() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void anomalyDetected(int id, qreal deviation);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawAnomalyList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AnomalyEntry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
