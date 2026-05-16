#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SummitEntry {
    int id; QString peak; QString category; QString approach;
    qreal altitude; int papers; bool summited; QColor color;
};
class PaperReadingSummit : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSummit(QWidget* parent = nullptr);
    void addEntry(const SummitEntry& entry);
    QList<SummitEntry> entries() const;
    int summitedCount() const;
    qreal avgAltitude() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void peakReached(int id, qreal altitude);
private slots:
    void onClimb();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMountainView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SummitEntry> entries_;
    QSettings settings_;
    QPushButton* climbBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
