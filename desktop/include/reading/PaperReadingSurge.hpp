#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SurgeEntry {
    int id; QString period; QString category; QString peak;
    qreal intensity; int papers; bool spike; QColor color;
};
class PaperReadingSurge : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSurge(QWidget* parent = nullptr);
    void addEntry(const SurgeEntry& entry);
    QList<SurgeEntry> entries() const;
    int spikeCount() const;
    qreal avgIntensity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void surgeDetected(int id, qreal intensity);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSurgeChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SurgeEntry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
