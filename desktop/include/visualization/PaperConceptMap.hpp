#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ConceptNode {
    int id{-1};
    QString label;
    QString category;
    QPointF pos;
    QColor color;
    qreal radius{24.0};
    int frequency{1};
};

struct ConceptEdge {
    int fromId{-1};
    int toId{-1};
    qreal weight{1.0};
};

class PaperConceptMap : public QWidget {
    Q_OBJECT

public:
    explicit PaperConceptMap(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void addConcept(const ConceptNode& node);
    void addConnection(int fromId, int toId, qreal weight = 1.0);
    void extractFromText(const QString& text);
    void clear();
    QList<ConceptNode> concepts() const;

signals:
    void conceptClicked(int id, const QString& label);
    void conceptAdded(int id);

private slots:
    void onLayoutChanged(int index);
    void onFitView();
    void onConceptListClicked();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    int hitTest(const QPointF& pos);
    void forceDirectedLayout();
    void circularLayout();

    QComboBox* layoutCombo_{nullptr};
    QPushButton* fitBtn_{nullptr};
    QListWidget* conceptList_{nullptr};
    QLabel* statsLabel_{nullptr};
    QLabel* paperLabel_{nullptr};

    QList<ConceptNode> concepts_;
    QList<ConceptEdge> edges_;
    QPointF offset_;
    qreal zoom_{1.0};
    bool panning_{false};
    QPointF lastMousePos_;
    int selectedId_{-1};
    int nextId_{1};
};
