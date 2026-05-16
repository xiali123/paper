#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BulletEntry {
    int id; QString label; QString category; QString metric;
    qreal actual; qreal target; qreal benchmark; bool onTrack; QColor color;
};
class PaperBulletGraph : public QWidget {
    Q_OBJECT
public:
    explicit PaperBulletGraph(QWidget* parent = nullptr);
    void addEntry(const BulletEntry& entry);
    QList<BulletEntry> entries() const;
    int onTrackCount() const;
    qreal avgActual() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bulletClicked(int id, qreal actual);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBulletChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BulletEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
