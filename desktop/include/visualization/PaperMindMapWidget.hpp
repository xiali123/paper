#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVector>
#include <QMap>
#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QSplitter>

struct MindMapNode {
    int id{-1};
    QString label;
    QPointF pos;
    QColor color;
    qreal radius{20.0};
    int paperId{-1};
    bool expanded{true};
    QList<int> children;
};

struct MindMapEdge {
    int from{-1};
    int to{-1};
    QString label;
    QColor color{QColor(180, 180, 180)};
};

class PaperMindMapWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperMindMapWidget(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void addRelation(int paperA, int paperB, const QString& relation = "cites");
    void clearMap();
    void autoLayout();
    void expandAll();
    void collapseAll();

    QSize sizeHint() const override { return QSize(700, 500); }

signals:
    void nodeClicked(int paperId);
    void nodeDoubleClicked(int paperId);
    void edgeClicked(int fromId, int toId);
    void layoutChanged();

private slots:
    void onLayoutChanged(int index);
    void onZoomChanged(int value);
    void onFitView();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupUI();
    void drawNode(QPainter& p, const MindMapNode& node);
    void drawEdge(QPainter& p, const MindMapEdge& edge);
    void layoutRadial();
    void layoutForceDirected();
    void layoutTree();
    int hitTest(const QPointF& pos);
    void refreshOutline();

    QSplitter* splitter_{nullptr};
    QWidget* canvas_{nullptr};
    QTreeWidget* outlineTree_{nullptr};
    QComboBox* layoutCombo_{nullptr};
    QSlider* zoomSlider_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* fitBtn_{nullptr};

    QMap<int, MindMapNode> nodes_;
    QList<MindMapEdge> edges_;
    int nextNodeId_{1};

    QPointF offset_;
    qreal zoom_{1.0};
    int dragNode_{-1};
    QPointF lastMousePos_;
    bool panning_{false};
};
