#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DnsResolverEntry {
    int id; QString domain; QString category; QString record;
    qreal ttl; int queries; bool resolved; QColor color;
};
class PaperDnsResolver : public QWidget {
    Q_OBJECT
public:
    explicit PaperDnsResolver(QWidget* parent = nullptr);
    void addEntry(const DnsResolverEntry& entry);
    QList<DnsResolverEntry> entries() const;
    int resolvedCount() const;
    qreal avgTtl() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void domainResolved(int id, qreal ttl);
private slots:
    void onResolve();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawResolverView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DnsResolverEntry> entries_;
    QSettings settings_;
    QPushButton* resolveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
