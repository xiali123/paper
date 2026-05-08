#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct CitationStat {
    int paperId{-1};
    QString title;
    int citationCount{0};
    int selfCitations{0};
    QString year;
    qreal citationsPerYear{0};
    qreal hIndex{0};
};

class PaperCitationCounter : public QWidget {
    Q_OBJECT

public:
    explicit PaperCitationCounter(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void addStat(const CitationStat& stat);
    QList<CitationStat> stats() const;
    QList<CitationStat> topCited(int limit = 10) const;
    qreal hIndex() const;
    int totalCitations() const;

signals:
    void paperClicked(int paperId, int citations);

private slots:
    void onSortChanged(int index);
    void onRefresh();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawBarChart(QPainter& p, const QRect& rect);
    void drawYearChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();

    QComboBox* sortCombo_{nullptr};
    QPushButton* refreshBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<CitationStat> stats_;
};
