#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CertGuard2Entry {
    int id; QString domain; QString category; QString algorithm;
    qreal strength; int certs; bool weak; QColor color;
};
class PaperCertGuard2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperCertGuard2(QWidget* parent = nullptr);
    void addEntry(const CertGuard2Entry& entry);
    QList<CertGuard2Entry> entries() const;
    int weakCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void certScanned(int id, qreal strength);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGuardView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CertGuard2Entry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
