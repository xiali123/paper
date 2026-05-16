#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct AssetEntry {
    int id;
    QString name;
    QString category;
    QString type;
    qreal value;
    QString acquired;
    bool active;
    bool depreciating;
    QColor color;
};

class PaperAssetManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperAssetManager(QWidget* parent = nullptr);
    void addEntry(const AssetEntry& entry);
    QList<AssetEntry> entries() const;
    int activeCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void assetAdded(int id, qreal value);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawAssetList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AssetEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
