#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SessionSyncEntry {
    int id; QString device; QString category; QString paper;
    qreal progress; int pagesSynced; bool synced; QColor color;
};
class PaperReadingSessionSync : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSessionSync(QWidget* parent = nullptr);
    void addEntry(const SessionSyncEntry& entry);
    QList<SessionSyncEntry> entries() const;
    int syncedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void syncComplete(int id, qreal progress);
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
    QList<SessionSyncEntry> entries_;
    QSettings settings_;
    QPushButton* syncBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
