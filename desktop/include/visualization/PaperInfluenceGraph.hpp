#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct InfluenceNode {
    int id{-1};
    QString author;
    QString field;
    qreal influence{0};
    int collaborators{0};
    int papers{0};
    qreal score{0};
    QColor color;
};

class PaperInfluenceGraph : public QWidget {
    Q_OBJECT

public:
    explicit PaperInfluenceGraph(QWidget* parent = nullptr);

    void addNode(const InfluenceNode& node);
    QList<InfluenceNode> nodes() const;
    qreal averageInfluence() const;
    int topInfluencer() const;

signals:
    void nodeSelected(int nodeId);
    void graphUpdated(int count);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawNetworkGraph(QPainter& p, const QRect& rect);
    void drawInfluenceBars(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<InfluenceNode> nodes_;
    int selectedNode_{-1};
    QSettings settings_;
};
