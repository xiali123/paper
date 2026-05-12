#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CacheEntry {
    int id; QString key; QString category; QString tier;
    qreal hitRate; int size; QString ttl; bool warm; QColor color;
};
class PaperCacheWarmer : public QWidget {
    Q_OBJECT
public:
    explicit PaperCacheWarmer(QWidget* parent = nullptr);
    void addEntry(const CacheEntry& entry);
    QList<CacheEntry> entries() const;
    int warmCount() const;
    qreal avgHitRate() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void cacheWarmed(int id, qreal hitRate);
private slots:
    void onWarm();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCacheList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CacheEntry> entries_;
    QSettings settings_;
    QPushButton* warmBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
