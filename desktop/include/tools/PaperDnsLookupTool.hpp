#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DnsEntry {
    int id; QString domain; QString category; QString recordType;
    qreal latency; int ttl; bool resolved; QColor color;
};
class PaperDnsLookupTool : public QWidget {
    Q_OBJECT
public:
    explicit PaperDnsLookupTool(QWidget* parent = nullptr);
    void addEntry(const DnsEntry& entry);
    QList<DnsEntry> entries() const;
    int resolvedCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void lookupComplete(int id, qreal latency);
private slots:
    void onLookup();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDnsList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DnsEntry> entries_;
    QSettings settings_;
    QPushButton* lookupBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
