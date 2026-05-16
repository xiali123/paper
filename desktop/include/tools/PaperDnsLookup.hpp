#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DnsLookupEntry {
    int id; QString domain; QString category; QString record;
    qreal latency; int records; bool resolved; QColor color;
};
class PaperDnsLookup : public QWidget {
    Q_OBJECT
public:
    explicit PaperDnsLookup(QWidget* parent = nullptr);
    void addEntry(const DnsLookupEntry& entry);
    QList<DnsLookupEntry> entries() const;
    int resolvedCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dnsResolved(int id, qreal latency);
private slots:
    void onLookup();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLookupView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DnsLookupEntry> entries_;
    QSettings settings_;
    QPushButton* lookupBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
