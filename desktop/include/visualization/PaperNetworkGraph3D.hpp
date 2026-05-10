#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct GraphNode3D {
    int id;
    QString label;
    QString category;
    qreal x, y, z;
    qreal size;
    int connections;
    qreal weight;
    bool hub;
    QColor color;
};

class PaperNetworkGraph3D : public QWidget {
    Q_OBJECT
public:
    explicit PaperNetworkGraph3D(QWidget* parent = nullptr);
    void addEntry(const GraphNode3D& entry);
    QList<GraphNode3D> entries() const;
    int hubCount() const;
    qreal avgConnections() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void graphGenerated(int id, int connections);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawGraphView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<GraphNode3D> entries_;
};
