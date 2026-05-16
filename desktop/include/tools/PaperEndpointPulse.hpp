#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EndpointPulseEntry {
    int id; QString endpoint; QString category; QString method;
    qreal responseTime; int requests; bool healthy; QColor color;
};
class PaperEndpointPulse : public QWidget {
    Q_OBJECT
public:
    explicit PaperEndpointPulse(QWidget* parent = nullptr);
    void addEntry(const EndpointPulseEntry& entry);
    QList<EndpointPulseEntry> entries() const;
    int healthyCount() const;
    qreal avgResponseTime() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pulseChecked(int id, qreal responseTime);
private slots:
    void onPulse();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPulseView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EndpointPulseEntry> entries_;
    QSettings settings_;
    QPushButton* pulseBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
