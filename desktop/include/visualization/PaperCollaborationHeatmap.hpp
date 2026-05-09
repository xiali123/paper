#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct CollabCell {
    int row{0};
    int col{0};
    qreal strength{0};
    QColor color;
};

class PaperCollaborationHeatmap : public QWidget {
    Q_OBJECT

public:
    explicit PaperCollaborationHeatmap(QWidget* parent = nullptr);

    void addAuthor(const QString& name);
    void setCell(int row, int col, qreal strength);
    QStringList authors() const;
    QList<CollabCell> cells() const;
    qreal avgCollaboration() const;

signals:
    void cellClicked(int row, int col);
    void heatmapUpdated(int authors);

private slots:
    void onGenerate();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawHeatmap(QPainter& p, const QRect& rect);
    void drawLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* generateBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QStringList authors_;
    QList<CollabCell> cells_;
    QSettings settings_;
};
