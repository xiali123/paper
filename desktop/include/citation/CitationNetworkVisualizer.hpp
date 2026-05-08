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

struct CitationNode {
    int paperId{-1};
    QString title;
    QString author;
    int year{0};
    QPointF pos;
    QColor color;
    qreal radius{20.0};
    int citationCount{0};
};

struct CitationEdge {
    int fromId{-1};
    int toId{-1};
    QString type{"cites"};
};

class CitationNetworkVisualizer : public QWidget {
    Q_OBJECT

public:
    explicit CitationNetworkVisualizer(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void addCitation(int fromId, int toId, const QString& type = "cites");
    void clear();
    void computeLayout();

signals:
    void nodeClicked(int paperId);
    void nodeHovered(int paperId, const QString& title);

private slots:
    void onLayoutChanged(int index);
    void onFitView();
    void onResetView();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    int hitTest(const QPointF& pos);

    QComboBox* layoutCombo_{nullptr};
    QPushButton* fitBtn_{nullptr};
    QPushButton* resetBtn_{nullptr};
    QListWidget* infoList_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<CitationNode> nodes_;
    QList<CitationEdge> edges_;
    QPointF offset_;
    qreal zoom_{1.0};
    bool panning_{false};
    QPointF lastMousePos_;
};
