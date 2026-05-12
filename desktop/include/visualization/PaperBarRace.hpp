#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BarRaceEntry {
    int id; QString label; QString category; QString metric;
    qreal value; int rank; bool leading; QColor color;
};
class PaperBarRace : public QWidget {
    Q_OBJECT
public:
    explicit PaperBarRace(QWidget* parent = nullptr);
    void addEntry(const BarRaceEntry& entry);
    QList<BarRaceEntry> entries() const;
    int leadingCount() const;
    qreal topValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void raceUpdated(int id, qreal value);
private slots:
    void onAnimate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBarRace(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BarRaceEntry> entries_;
    QSettings settings_;
    QPushButton* animateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
