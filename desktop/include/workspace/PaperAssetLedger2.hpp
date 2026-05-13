#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct AssetLedger2Entry {
    int id; QString asset; QString category; QString status;
    qreal value; int transactions; bool active; QColor color;
};
class PaperAssetLedger2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperAssetLedger2(QWidget* parent = nullptr);
    void addEntry(const AssetLedger2Entry& entry);
    QList<AssetLedger2Entry> entries() const;
    int activeCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void assetTraded(int id, qreal value);
private slots:
    void onTrade();
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
    QList<AssetLedger2Entry> entries_;
    QSettings settings_;
    QPushButton* tradeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
