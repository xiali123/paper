#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ElevEntry {
    int id; QString paper; QString category; QString tier;
    qreal level; qreal gain; qreal base; bool summit; QColor color;
};
class PaperReadingElevation : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingElevation(QWidget* parent = nullptr);
    void addEntry(const ElevEntry& entry);
    QList<ElevEntry> entries() const;
    int summitCount() const;
    qreal avgGain() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void elevationMeasured(int id, qreal gain);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawElevationView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ElevEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
