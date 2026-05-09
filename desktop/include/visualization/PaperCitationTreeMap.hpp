#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct TreeMapNode {
    int id{-1};
    QString label;
    qreal weight{0};
    QString category;
    QRectF rect;
    QColor color;
};

class PaperCitationTreeMap : public QWidget {
    Q_OBJECT

public:
    explicit PaperCitationTreeMap(QWidget* parent = nullptr);

    void addNode(const TreeMapNode& node);
    QList<TreeMapNode> nodes() const;
    QMap<QString, qreal> categoryWeights() const;
    qreal totalWeight() const;

signals:
    void nodeClicked(int nodeId);
    void mapUpdated(int count);

private slots:
    void onGenerate();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTreeMap(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void computeLayout(const QRectF& rect, QList<TreeMapNode>& nodes);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* generateBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<TreeMapNode> nodes_;
    QSettings settings_;
};
