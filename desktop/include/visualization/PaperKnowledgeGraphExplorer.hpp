#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>

struct KnowledgeNode {
    int id;
    QString concept;
    QString category;
    qreal weight;
    int connections;
    QString description;
    QColor color;
};

class PaperKnowledgeGraphExplorer : public QWidget {
    Q_OBJECT
public:
    explicit PaperKnowledgeGraphExplorer(QWidget* parent = nullptr);
    void addNode(const KnowledgeNode& node);
    QList<KnowledgeNode> nodes() const;
    QMap<QString, int> categoryCounts() const;
    qreal avgWeight() const;
    int totalConnections() const;

signals:
    void graphUpdated(int nodeCount, int totalConnections);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onGenerate();
    void onClear();
    void drawGraphView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<KnowledgeNode> nodes_;
    QPushButton* addBtn_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLineEdit* searchField_;
    QLabel* infoLabel_;
};
