#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct VendorEntry {
    int id;
    QString vendorName;
    QString service;
    qreal rating;
    int reviews;
    QString category;
    qreal costScore;
    int papersServed;
    bool recommended;
    QColor color;
};

class PaperVendorRating : public QWidget {
    Q_OBJECT
public:
    explicit PaperVendorRating(QWidget* parent = nullptr);
    void addEntry(const VendorEntry& entry);
    QList<VendorEntry> entries() const;
    qreal avgRating() const;
    int recommendedCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void vendorRated(int id, qreal rating);
private slots:
    void onRate();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawVendorList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VendorEntry> entries_;
    QSettings settings_;
    QPushButton* rateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
