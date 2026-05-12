#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingZoneEntry {
    int id; QString zone; QString category; QString paper;
    qreal focus; int duration; bool deep; QColor color;
};
class PaperReadingZoneTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingZoneTracker(QWidget* parent = nullptr);
    void addEntry(const ReadingZoneEntry& entry);
    QList<ReadingZoneEntry> entries() const;
    int deepCount() const;
    qreal avgFocus() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void zoneEntered(int id, qreal focus);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawZoneMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingZoneEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
