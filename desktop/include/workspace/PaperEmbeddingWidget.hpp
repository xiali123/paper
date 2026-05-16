#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QMap>
#include <QList>
#include <QRandomGenerator>

struct EmbeddingPoint {
    int paperId{-1};
    QString title;
    QPointF pos;
    QColor color;
    QString cluster;
};

class PaperEmbeddingWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperEmbeddingWidget(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void setEmbeddings(const QMap<int, QPointF>& embeddings);
    void computeRandomEmbeddings();
    void colorByCluster(const QMap<int, QString>& clusters);
    void colorByYear(const QMap<int, int>& years);
    void clear();

    QSize sizeHint() const override { return QSize(600, 450); }

signals:
    void pointClicked(int paperId);
    void clusterSelected(const QString& cluster);
    void recomputeRequested(const QString& method);

private slots:
    void onMethodChanged(int index);
    void onFitView();
    void onColorByChanged(int index);
    void onRecompute();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void setupUI();
    int hitTest(const QPointF& pos);

    QComboBox* methodCombo_{nullptr};
    QComboBox* colorByCombo_{nullptr};
    QPushButton* recomputeBtn_{nullptr};
    QPushButton* fitBtn_{nullptr};
    QListWidget* legendList_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<EmbeddingPoint> points_;
    QPointF offset_;
    qreal zoom_{1.0};
    bool panning_{false};
    QPointF lastMousePos_;
};
