#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SyncHubEntry {
    int id; QString session; QString category; QString device;
    qreal progress; int synced; bool current; QColor color;
};
class PaperReadingSyncHub : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSyncHub(QWidget* parent = nullptr);
    void addEntry(const SyncHubEntry& entry);
    QList<SyncHubEntry> entries() const;
    int currentCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sessionSynced(int id, qreal progress);
private slots:
    void onSync();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSyncView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SyncHubEntry> entries_;
    QSettings settings_;
    QPushButton* syncBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
