#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DnsLookup2Entry {
    int id; QString domain; QString category; QString record;
    qreal latency; int queries; bool resolved; QColor color;
};
class PaperDnsLookup2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperDnsLookup2(QWidget* parent = nullptr);
    void addEntry(const DnsLookup2Entry& entry);
    QList<DnsLookup2Entry> entries() const;
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
    QList<DnsLookup2Entry> entries_;
    QSettings settings_;
    QPushButton* lookupBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
