#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CapacityEntry {
    int id; QString resource; QString category; QString period;
    qreal allocated; qreal used; qreal available; bool overloaded; QColor color;
};
class PaperCapacityPlanner : public QWidget {
    Q_OBJECT
public:
    explicit PaperCapacityPlanner(QWidget* parent = nullptr);
    void addEntry(const CapacityEntry& entry);
    QList<CapacityEntry> entries() const;
    int overloadedCount() const;
    qreal avgUtilization() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void capacityUpdated(int id, qreal used);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCapacityList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CapacityEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
