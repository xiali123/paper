#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HealthEntry {
    int id; QString service; QString category; QString status;
    qreal uptime; qreal responseTime; int checks; bool healthy; QColor color;
};
class PaperHealthCheck : public QWidget {
    Q_OBJECT
public:
    explicit PaperHealthCheck(QWidget* parent = nullptr);
    void addEntry(const HealthEntry& entry);
    QList<HealthEntry> entries() const;
    int healthyCount() const;
    qreal avgResponseTime() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void healthChecked(int id, qreal responseTime);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHealthList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HealthEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
