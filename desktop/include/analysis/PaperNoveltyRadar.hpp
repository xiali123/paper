#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct NoveltyEntry {
    int id; QString paper; QString category; QString dimension;
    qreal score; int citations; bool breakthrough; QColor color;
};
class PaperNoveltyRadar : public QWidget {
    Q_OBJECT
public:
    explicit PaperNoveltyRadar(QWidget* parent = nullptr);
    void addEntry(const NoveltyEntry& entry);
    QList<NoveltyEntry> entries() const;
    int breakthroughCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void noveltyDetected(int id, qreal score);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRadarChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<NoveltyEntry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
