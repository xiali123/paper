#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CertRenewerEntry {
    int id; QString domain; QString category; QString issuer;
    qreal daysLeft; int renewals; bool expiring; QColor color;
};
class PaperCertRenewer : public QWidget {
    Q_OBJECT
public:
    explicit PaperCertRenewer(QWidget* parent = nullptr);
    void addEntry(const CertRenewerEntry& entry);
    QList<CertRenewerEntry> entries() const;
    int expiringCount() const;
    qreal avgDaysLeft() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void certRenewed(int id, qreal daysLeft);
private slots:
    void onRenew();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRenewerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CertRenewerEntry> entries_;
    QSettings settings_;
    QPushButton* renewBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
