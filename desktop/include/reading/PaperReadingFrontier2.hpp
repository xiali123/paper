#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingFrontier2Entry {
    int id; QString territory; QString category; QString explorer;
    qreal coverage; int discoveries; bool mapped; QColor color;
};
class PaperReadingFrontier2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingFrontier2(QWidget* parent = nullptr);
    void addEntry(const ReadingFrontier2Entry& entry);
    QList<ReadingFrontier2Entry> entries() const;
    int mappedCount() const;
    qreal avgCoverage() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void territoryMapped(int id, qreal coverage);
private slots:
    void onExplore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFrontierView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingFrontier2Entry> entries_;
    QSettings settings_;
    QPushButton* exploreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
