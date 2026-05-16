#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct SankeyEntry {
    int id;
    QString source;
    QString target;
    qreal flow;
    QString category;
    qreal efficiency;
    int papersFlow;
    QString layer;
    bool major;
    QColor color;
};

class PaperSankeyDiagram : public QWidget {
    Q_OBJECT
public:
    explicit PaperSankeyDiagram(QWidget* parent = nullptr);
    void addEntry(const SankeyEntry& entry);
    QList<SankeyEntry> entries() const;
    qreal totalFlow() const;
    int majorCount() const;
    QMap<QString, int> layerCounts() const;

signals:
    void sankeyGenerated(int id, qreal flow);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawSankeyView(QPainter& p, const QRect& rect);
    void drawLayerLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<SankeyEntry> entries_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* layerCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
