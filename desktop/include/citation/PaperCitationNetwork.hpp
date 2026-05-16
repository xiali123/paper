#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct CitationNode {
    int id{-1};
    QString title;
    int year{0};
    int citations{0};
    QString cluster;
    QColor color;
    qreal x{0};
    qreal y{0};
    qreal vx{0};
    qreal vy{0};
};

struct CitationEdge {
    int from{-1};
    int to{-1};
    qreal weight{0};
};

class PaperCitationNetwork : public QWidget {
    Q_OBJECT

public:
    explicit PaperCitationNetwork(QWidget* parent = nullptr);

    void addNode(const CitationNode& node);
    void addEdge(int from, int to, qreal weight = 1.0);
    QList<CitationNode> nodes() const;
    QList<CitationEdge> edges() const;
    QMap<QString, int> clusterCounts() const;

signals:
    void nodeClicked(int nodeId);
    void networkUpdated(int nodes, int edges);

private slots:
    void onGenerate();
    void onLayout();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawNetwork(QPainter& p, const QRect& rect);
    void drawLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void forceDirectedLayout(int iterations = 50);

    QPushButton* generateBtn_{nullptr};
    QPushButton* layoutBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<CitationNode> nodes_;
    QList<CitationEdge> edges_;
    int selectedNode_{-1};
};
