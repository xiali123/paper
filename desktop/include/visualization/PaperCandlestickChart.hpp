#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct CandleEntry {
    int id;
    QString label;
    QString category;
    qreal open;
    qreal high;
    qreal low;
    qreal close;
    qreal volume;
    bool bullish;
    QColor color;
};

class PaperCandlestickChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperCandlestickChart(QWidget* parent = nullptr);
    void addEntry(const CandleEntry& entry);
    QList<CandleEntry> entries() const;
    int bullishCount() const;
    qreal maxPrice() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void chartGenerated(int id, qreal close);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCandleView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<CandleEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
