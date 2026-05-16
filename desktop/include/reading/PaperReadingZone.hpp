#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ZoneEntry {
    int id; QString zone; QString category; QString activity;
    qreal focus; int minutes; bool deepWork; QColor color;
};
class PaperReadingZone : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingZone(QWidget* parent = nullptr);
    void addEntry(const ZoneEntry& entry);
    QList<ZoneEntry> entries() const;
    int deepWorkCount() const;
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
    void drawZoneView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ZoneEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
