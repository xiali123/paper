#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VitalEntry {
    int id; QString paper; QString category; QString metric;
    qreal value; qreal baseline; qreal delta; bool healthy; QColor color;
};
class PaperReadingVitals : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingVitals(QWidget* parent = nullptr);
    void addEntry(const VitalEntry& entry);
    QList<VitalEntry> entries() const;
    int healthyCount() const;
    qreal avgDelta() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void vitalMeasured(int id, qreal value);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVitalList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VitalEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
