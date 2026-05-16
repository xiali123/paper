#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WatchEntry {
    int id; QString path; QString category; QString event;
    int count; qreal frequency; QString lastSeen; bool active; QColor color;
};
class PaperFileWatcher : public QWidget {
    Q_OBJECT
public:
    explicit PaperFileWatcher(QWidget* parent = nullptr);
    void addEntry(const WatchEntry& entry);
    QList<WatchEntry> entries() const;
    int activeCount() const;
    qreal avgFrequency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void fileWatched(int id, qreal frequency);
private slots:
    void onWatch();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawWatchList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WatchEntry> entries_;
    QSettings settings_;
    QPushButton* watchBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
