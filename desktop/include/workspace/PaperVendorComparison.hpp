#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct VendorEntry {
    int id;
    QString vendorName;
    QString service;
    qreal price;
    qreal rating;
    QString reliability;
    int contracts;
    qreal savings;
    bool preferred;
    QColor color;
};

class PaperVendorComparison : public QWidget {
    Q_OBJECT
public:
    explicit PaperVendorComparison(QWidget* parent = nullptr);
    void addEntry(const VendorEntry& entry);
    QList<VendorEntry> entries() const;
    qreal avgRating() const;
    int preferredCount() const;
    QMap<QString, int> serviceCounts() const;

signals:
    void vendorCompared(int id, qreal price);

private slots:
    void onCompare();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawVendorList(QPainter& p, const QRect& rect);
    void drawServiceChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* compareBtn_;
    QPushButton* clearBtn_;
    QComboBox* serviceCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<VendorEntry> entries_;
};
