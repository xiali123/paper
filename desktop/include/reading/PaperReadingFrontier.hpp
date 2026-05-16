#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct FrontierEntry {
    int id; QString domain; QString category; QString discovery;
    qreal depth; int papers; bool uncharted; QColor color;
};
class PaperReadingFrontier : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingFrontier(QWidget* parent = nullptr);
    void addEntry(const FrontierEntry& entry);
    QList<FrontierEntry> entries() const;
    int unchartedCount() const;
    qreal avgDepth() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void frontierExplored(int id, qreal depth);
private slots:
    void onExplore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFrontierMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FrontierEntry> entries_;
    QSettings settings_;
    QPushButton* exploreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
