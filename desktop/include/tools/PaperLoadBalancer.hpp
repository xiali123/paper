#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LoadBalancerEntry {
    int id; QString server; QString category; QString algorithm;
    qreal load; int connections; bool healthy; QColor color;
};
class PaperLoadBalancer : public QWidget {
    Q_OBJECT
public:
    explicit PaperLoadBalancer(QWidget* parent = nullptr);
    void addEntry(const LoadBalancerEntry& entry);
    QList<LoadBalancerEntry> entries() const;
    int healthyCount() const;
    qreal avgLoad() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void serverChecked(int id, qreal load);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBalancerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LoadBalancerEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
