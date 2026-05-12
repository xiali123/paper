#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ArchipelagoEntry {
    int id; QString island; QString category; QString region;
    qreal depth; int papers; bool explored; QColor color;
};
class PaperReadingArchipelago : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingArchipelago(QWidget* parent = nullptr);
    void addEntry(const ArchipelagoEntry& entry);
    QList<ArchipelagoEntry> entries() const;
    int exploredCount() const;
    qreal avgDepth() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void islandExplored(int id, qreal depth);
private slots:
    void onExplore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawIslandMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ArchipelagoEntry> entries_;
    QSettings settings_;
    QPushButton* exploreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
