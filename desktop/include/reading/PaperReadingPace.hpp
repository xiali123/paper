#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PaceEntry {
    int id; QString paper; QString category; QString pace;
    qreal speed; qreal consistency; qreal retention; bool onTrack; QColor color;
};
class PaperReadingPace : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingPace(QWidget* parent = nullptr);
    void addEntry(const PaceEntry& entry);
    QList<PaceEntry> entries() const;
    int onTrackCount() const;
    qreal avgSpeed() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void paceMeasured(int id, qreal speed);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPaceView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PaceEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
