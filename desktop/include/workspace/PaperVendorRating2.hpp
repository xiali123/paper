#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VendorRatingEntry {
    int id; QString vendor; QString category; QString service;
    qreal rating; int reviews; bool recommended; QColor color;
};
class PaperVendorRating2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperVendorRating2(QWidget* parent = nullptr);
    void addEntry(const VendorRatingEntry& entry);
    QList<VendorRatingEntry> entries() const;
    int recommendedCount() const;
    qreal avgRating() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void vendorRated(int id, qreal rating);
private slots:
    void onRate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVendorList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VendorRatingEntry> entries_;
    QSettings settings_;
    QPushButton* rateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
