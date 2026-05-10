#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct FunnelEntry {
    int id;
    QString stage;
    int count;
    qreal percentage;
    QString category;
    int dropoff;
    qreal conversionRate;
    QString color_label;
    int order;
    bool bottleneck;
    QColor color;
};

class PaperFunnelChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperFunnelChart(QWidget* parent = nullptr);
    void addEntry(const FunnelEntry& entry);
    QList<FunnelEntry> entries() const;
    int totalCount() const;
    int bottleneckCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void funnelGenerated(int id, int count);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawFunnelView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<FunnelEntry> entries_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
