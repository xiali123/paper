#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct WaterfallEntry {
    int id;
    QString label;
    qreal value;
    qreal cumulative;
    QString category;
    bool positive;
    int order;
    QString period;
    qreal percentage;
    QColor color;
};

class PaperWaterfallChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperWaterfallChart(QWidget* parent = nullptr);
    void addEntry(const WaterfallEntry& entry);
    QList<WaterfallEntry> entries() const;
    qreal totalValue() const;
    int positiveCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void waterfallGenerated(int id, qreal value);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawWaterfallView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<WaterfallEntry> entries_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* periodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
