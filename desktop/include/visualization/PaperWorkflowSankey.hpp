#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct SankeyNode {
    int id{-1};
    QString label;
    QString type; // "source", "process", "output"
    qreal flow{0};
    QColor color;
};

class PaperWorkflowSankey : public QWidget {
    Q_OBJECT

public:
    explicit PaperWorkflowSankey(QWidget* parent = nullptr);

    void addNode(const SankeyNode& node);
    QList<SankeyNode> nodes() const;
    QMap<QString, qreal> typeFlows() const;
    qreal totalFlow() const;

signals:
    void flowUpdated(qreal total);
    void nodeClicked(int id);

private slots:
    void onGenerate();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawSankeyDiagram(QPainter& p, const QRect& rect);
    void drawFlowBars(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* generateBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<SankeyNode> nodes_;
    QSettings settings_;
};
