#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

struct NetworkNode {
    int id;
    QString paperTitle;
    int citationCount;
    int year;
    qreal centrality;
    QString cluster;
    int degree;
    QColor color;
};

class PaperCitationNetworkD3 : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationNetworkD3(QWidget* parent = nullptr);
    void addNode(const NetworkNode& node);
    QList<NetworkNode> nodes() const;
    QMap<QString, int> clusterCounts() const;
    qreal avgCentrality() const;
    int totalDegree() const;

signals:
    void networkUpdated(int nodeCount);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawNetworkView(QPainter& p, const QRect& rect);
    void drawClusterChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<NetworkNode> nodes_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLineEdit* searchField_;
    QLabel* infoLabel_;
};
