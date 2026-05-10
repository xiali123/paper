#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct AnnotationSyncEntry {
    int id;
    QString paperTitle;
    QString annotationType;
    QString content;
    QString device;
    qint64 timestamp;
    QString status;
    qreal syncProgress;
    QString colorTag;
    bool synced;
    QColor color;
};

class PaperAnnotationSync : public QWidget {
    Q_OBJECT
public:
    explicit PaperAnnotationSync(QWidget* parent = nullptr);
    void addEntry(const AnnotationSyncEntry& entry);
    QList<AnnotationSyncEntry> entries() const;
    int syncedCount() const;
    int pendingCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void syncCompleted(int id, bool success);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onSync();
    void onClear();
    void drawSyncList(QPainter& p, const QRect& rect);
    void drawDeviceChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QPushButton* syncBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<AnnotationSyncEntry> entries_;
    QSettings settings_;
};
