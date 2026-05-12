#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ResourceGanttEntry {
    int id; QString resource; QString category; QString assignee;
    qreal utilization; int tasks; bool available; QColor color;
};
class PaperResourceGantt : public QWidget {
    Q_OBJECT
public:
    explicit PaperResourceGantt(QWidget* parent = nullptr);
    void addEntry(const ResourceGanttEntry& entry);
    QList<ResourceGanttEntry> entries() const;
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
    void drawGanttView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ResourceGanttEntry> entries_;
    QSettings settings_;
    QPushButton* allocateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
