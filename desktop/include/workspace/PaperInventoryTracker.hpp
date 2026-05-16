#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct InventoryEntry {
    int id;
    QString itemName;
    QString category;
    int quantity;
    int threshold;
    QString location;
    QString status;
    qreal value;
    bool lowStock;
    QColor color;
};

class PaperInventoryTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperInventoryTracker(QWidget* parent = nullptr);
    void addEntry(const InventoryEntry& entry);
    QList<InventoryEntry> entries() const;
    qreal totalValue() const;
    int lowStockCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void inventoryUpdated(int id, int quantity);

private slots:
    void onTrack();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawInventoryList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<InventoryEntry> entries_;
};
