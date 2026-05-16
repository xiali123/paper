#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ResourceEntry {
    int id; QString resource; QString category; QString type;
    int allocated; int used; qreal utilization; bool available; QColor color;
};
class PaperResourcePool : public QWidget {
    Q_OBJECT
public:
    explicit PaperResourcePool(QWidget* parent = nullptr);
    void addEntry(const ResourceEntry& entry);
    QList<ResourceEntry> entries() const;
    int availableCount() const;
    qreal avgUtilization() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void resourceAllocated(int id, qreal utilization);
private slots:
    void onAllocate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawResourceList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ResourceEntry> entries_;
    QSettings settings_;
    QPushButton* allocateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
