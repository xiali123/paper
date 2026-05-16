#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct GeoPoint {
    int paperId{-1};
    QString institution;
    QString country;
    qreal latitude{0};
    qreal longitude{0};
    int paperCount{0};
    QColor color;
};

class PaperGeographyMap : public QWidget {
    Q_OBJECT

public:
    explicit PaperGeographyMap(QWidget* parent = nullptr);

    void addPoint(const GeoPoint& point);
    QList<GeoPoint> points() const;
    QMap<QString, int> countryCounts() const;
    int totalInstitutions() const;

signals:
    void regionClicked(const QString& country);
    void mapUpdated(int points);

private slots:
    void onGenerate();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawWorldMap(QPainter& p, const QRect& rect);
    void drawRegionChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* generateBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<GeoPoint> points_;
    QSettings settings_;
};
