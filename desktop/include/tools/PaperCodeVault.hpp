#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VaultEntry {
    int id; QString title; QString category; QString language;
    int lines; qreal quality; QString date; bool starred; QColor color;
};
class PaperCodeVault : public QWidget {
    Q_OBJECT
public:
    explicit PaperCodeVault(QWidget* parent = nullptr);
    void addEntry(const VaultEntry& entry);
    QList<VaultEntry> entries() const;
    int starredCount() const;
    qreal avgQuality() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void snippetStored(int id, qreal quality);
private slots:
    void onStore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVaultList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VaultEntry> entries_;
    QSettings settings_;
    QPushButton* storeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
