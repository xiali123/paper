#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct CitationRadarEntry {
    int paperId{-1};
    QString title;
    qreal selfCite{0};
    qreal crossCite{0};
    qreal externalCite{0};
    qreal impactFactor{0};
    qreal hIndex{0};
    int totalCitations{0};
    QColor color;
};

class PaperCitationRadarChart : public QWidget {
    Q_OBJECT

public:
    explicit PaperCitationRadarChart(QWidget* parent = nullptr);

    void addEntry(const CitationRadarEntry& entry);
    QList<CitationRadarEntry> entries() const;
    qreal averageImpact() const;
    int topCitedPaper() const;

signals:
    void entrySelected(int paperId);
    void chartUpdated(int count);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawRadarChart(QPainter& p, const QRect& rect);
    void drawCitationBars(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<CitationRadarEntry> entries_;
    int selectedEntry_{-1};
    QSettings settings_;
};
