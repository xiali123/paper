#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CertGuardEntry {
    int id; QString domain; QString category; QString issuer;
    qreal daysLeft; int certificates; bool valid; QColor color;
};
class PaperCertGuard : public QWidget {
    Q_OBJECT
public:
    explicit PaperCertGuard(QWidget* parent = nullptr);
    void addEntry(const CertGuardEntry& entry);
    QList<CertGuardEntry> entries() const;
    int validCount() const;
    qreal avgDaysLeft() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void certChecked(int id, qreal daysLeft);
private slots:
    void onGuard();
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
    QList<CertGuardEntry> entries_;
    QSettings settings_;
    QPushButton* guardBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
