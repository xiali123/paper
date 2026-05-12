#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RhythmEntry {
    int id; QString period; QString category; QString pattern;
    qreal intensity; qreal consistency; qreal focus; bool peak; QColor color;
};
class PaperReadingRhythm : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingRhythm(QWidget* parent = nullptr);
    void addEntry(const RhythmEntry& entry);
    QList<RhythmEntry> entries() const;
    int peakCount() const;
    qreal avgIntensity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rhythmMeasured(int id, qreal intensity);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRhythmView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RhythmEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
