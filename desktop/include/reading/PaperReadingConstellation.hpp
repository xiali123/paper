#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ConstellationEntry {
    int id; QString star; QString category; QString cluster;
    qreal brightness; int connections; bool core; QColor color;
};
class PaperReadingConstellation : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingConstellation(QWidget* parent = nullptr);
    void addEntry(const ConstellationEntry& entry);
    QList<ConstellationEntry> entries() const;
    int coreCount() const;
    qreal avgBrightness() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void starMapped(int id, qreal brightness);
private slots:
    void onMap();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawConstellationView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConstellationEntry> entries_;
    QSettings settings_;
    QPushButton* mapBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
