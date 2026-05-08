#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QList>
#include <QPair>

struct NetworkNode {
    int id{-1};
    QString name;
    QString type;
    QPointF pos;
    qreal radius{16.0};
    QColor color;
    int degree{0};
};

struct NetworkEdge {
    int from{-1};
    int to{-1};
    qreal weight{1.0};
    QString label;
};

class PaperNetworkGraph : public QWidget {
    Q_OBJECT

public:
    explicit PaperNetworkGraph(QWidget* parent = nullptr);

    void addAuthor(const QString& name);
    void addCoauthorship(const QString& author1, const QString& author2, qreal weight = 1.0);
    void buildFromPapers(const QList<QPair<int, QStringList>>& papers);
    void clearGraph();
    void autoLayout();
    void setNodeSizeByDegree(bool enabled);

    QSize sizeHint() const override { return QSize(600, 450); }

signals:
    void nodeClicked(const QString& name, const QString& type);
    void nodeHovered(const QString& name);
    void layoutApplied();

private slots:
    void onLayoutChanged(int index);
    void onFitView();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupUI();
    void layoutForce();
    void layoutCircular();
    void layoutGrouped();
    int hitTest(const QPointF& pos);

    QComboBox* layoutCombo_{nullptr};
    QPushButton* fitBtn_{nullptr};
    QLabel* statsLabel_{nullptr};
    QListWidget* legendList_{nullptr};

    QMap<QString, NetworkNode> nodes_;
    QList<NetworkEdge> edges_;
    int nextNodeId_{1};

    QPointF offset_;
    qreal zoom_{1.0};
    int dragNode_{QString()};
    bool dragging_{false};
    QPointF lastMousePos_;
    bool sizeByDegree_{true};
};
