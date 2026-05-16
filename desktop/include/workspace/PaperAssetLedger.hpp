#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct AssetLedgerEntry {
    int id; QString asset; QString category; QString custodian;
    qreal value; int quantity; bool verified; QColor color;
};
class PaperAssetLedger : public QWidget {
    Q_OBJECT
public:
    explicit PaperAssetLedger(QWidget* parent = nullptr);
    void addEntry(const AssetLedgerEntry& entry);
    QList<AssetLedgerEntry> entries() const;
    int verifiedCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void assetRecorded(int id, qreal value);
private slots:
    void onRecord();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLedgerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AssetLedgerEntry> entries_;
    QSettings settings_;
    QPushButton* recordBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
