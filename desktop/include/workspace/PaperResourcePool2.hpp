#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ResourcePoolEntry {
    int id; QString resource; QString category; QString status;
    qreal utilization; int capacity; bool overloaded; QColor color;
};
class PaperResourcePool2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperResourcePool2(QWidget* parent = nullptr);
    void addEntry(const ResourcePoolEntry& entry);
    QList<ResourcePoolEntry> entries() const;
    int overloadedCount() const;
    qreal avgUtilization() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void resourceTracked(int id, qreal utilization);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPoolView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ResourcePoolEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
