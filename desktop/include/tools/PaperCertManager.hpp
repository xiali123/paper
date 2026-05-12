#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CertEntry {
    int id; QString domain; QString category; QString issuer;
    qreal daysLeft; int chainDepth; bool valid; QColor color;
};
class PaperCertManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperCertManager(QWidget* parent = nullptr);
    void addEntry(const CertEntry& entry);
    QList<CertEntry> entries() const;
    int validCount() const;
    qreal avgDaysLeft() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void certChecked(int id, qreal daysLeft);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCertList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CertEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
