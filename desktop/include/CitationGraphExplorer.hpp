#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QList>
#include <QMap>
#include <QRandomGenerator>

struct GraphNode {
    int id{-1};
    QString label;
    QString type{"paper"};
    QPointF pos;
    QColor color;
    qreal size{16.0};
    int depth{0};
};

struct GraphLink {
    int source{-1};
    int target{-1};
    QString relation{"cites"};
    qreal weight{1.0};
};

class CitationGraphExplorer : public QWidget {
    Q_OBJECT

public:
    explicit CitationGraphExplorer(QWidget* parent = nullptr);

    void loadFromPapers(const QList<QPair<int, QString>>& papers);
    void addNode(const GraphNode& node);
    void addLink(int source, int target, const QString& relation = "cites", qreal weight = 1.0);
    void clear();
    QList<GraphNode> nodes() const;
    QList<GraphLink> links() const;

signals:
    void nodeSelected(int id, const QString& label);
    void linkSelected(int source, int target);
    void explorationComplete(int nodeCount, int linkCount);

private slots:
    void onLayoutChanged(int index);
    void onDepthChanged(int value);
    void onFitView();
    void onNodeListClicked();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    int hitTestNode(const QPointF& pos);
    void forceLayout();
    void radialLayout();

    QComboBox* layoutCombo_{nullptr};
    QComboBox* depthCombo_{nullptr};
    QPushButton* fitBtn_{nullptr};
    QListWidget* nodeList_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<GraphNode> nodes_;
    QList<GraphLink> links_;
    QPointF offset_;
    qreal zoom_{1.0};
    bool panning_{false};
    QPointF lastMousePos_;
    int selectedNode_{-1};
};
