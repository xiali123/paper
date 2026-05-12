#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WebhookProbeEntry {
    int id; QString url; QString category; QString method;
    qreal responseTime; int deliveries; bool success; QColor color;
};
class PaperWebhookProbe : public QWidget {
    Q_OBJECT
public:
    explicit PaperWebhookProbe(QWidget* parent = nullptr);
    void addEntry(const WebhookProbeEntry& entry);
    QList<WebhookProbeEntry> entries() const;
    int successCount() const;
    qreal avgResponseTime() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void webhookTested(int id, qreal responseTime);
private slots:
    void onTest();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawProbeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WebhookProbeEntry> entries_;
    QSettings settings_;
    QPushButton* testBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
