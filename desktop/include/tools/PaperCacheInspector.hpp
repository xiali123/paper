#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct CacheEntry {
    int id;
    QString key;
    QString cacheType;
    int sizeKB;
    int hits;
    int misses;
    qreal hitRate;
    QString ttl;
    QString region;
    bool valid;
    QColor color;
};

class PaperCacheInspector : public QWidget {
    Q_OBJECT
public:
    explicit PaperCacheInspector(QWidget* parent = nullptr);
    void addEntry(const CacheEntry& entry);
    QList<CacheEntry> entries() const;
    qreal avgHitRate() const;
    int validCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void cacheInspected(int id, qreal hitRate);

private slots:
    void onInspect();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCacheList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<CacheEntry> entries_;
    QPushButton* inspectBtn_;
    QPushButton* clearBtn_;
    QComboBox* typeCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
