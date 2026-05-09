#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>

struct TrendCell {
    QString topic;
    QString year;
    qreal value{0};
    QColor color;
};

class PaperTrendHeatmap : public QWidget {
    Q_OBJECT

public:
    explicit PaperTrendHeatmap(QWidget* parent = nullptr);

    void setData(const QMap<QString, QMap<QString, qreal>>& data);
    QList<TrendCell> cells() const;
    QStringList topics() const;
    QStringList years() const;

signals:
    void cellClicked(const QString& topic, const QString& year);

private slots:
    void onGenerate();
    void onColorScheme(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawHeatmap(QPainter& p, const QRect& rect);
    void drawLegend(QPainter& p, const QRect& rect);
    void drawTopTrends(QPainter& p, const QRect& rect);
    void updateInfo();
    void rebuildCells();

    QComboBox* colorCombo_{nullptr};
    QPushButton* generateBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<TrendCell> cells_;
    QStringList topics_;
    QStringList years_;
    QMap<QString, QMap<QString, qreal>> rawData_;
};
