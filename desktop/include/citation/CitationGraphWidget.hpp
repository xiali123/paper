#pragma once

#include <QWidget>
#include <QMap>
#include <QList>
#include <QPointF>

struct Paper;

class CitationGraphWidget : public QWidget {
    Q_OBJECT

public:
    explicit CitationGraphWidget(QWidget* parent = nullptr);

    void setPapers(const QList<Paper>& papers);
    void setFocusPaper(int paperId);
    void clear();

signals:
    void paperClicked(int paperId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    struct GraphNode {
        int paperId{0};
        QString title;
        QString year;
        int citations{0};
        QPointF pos;
        QPointF velocity;
        float radius{20.0f};
        bool isFocus{false};
        QList<int> edges;
    };

    void layoutNodes();
    void forceDirectedLayout(int iterations = 50);

    QMap<int, GraphNode> nodes_;
    int focusPaperId_{0};
    QPointF offset_;
    float zoom_{1.0f};
    int draggingNode_{-1};
    QPointF lastMousePos_;
};
