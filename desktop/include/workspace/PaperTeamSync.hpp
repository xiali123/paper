#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SyncEntry {
    int id; QString task; QString category; QString assignee;
    qreal progress; qreal priority; QString dueDate; bool synced; QColor color;
};
class PaperTeamSync : public QWidget {
    Q_OBJECT
public:
    explicit PaperTeamSync(QWidget* parent = nullptr);
    void addEntry(const SyncEntry& entry);
    QList<SyncEntry> entries() const;
    int syncedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void taskSynced(int id, qreal progress);
private slots:
    void onSync();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSyncList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SyncEntry> entries_;
    QSettings settings_;
    QPushButton* syncBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
