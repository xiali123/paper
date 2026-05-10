#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct TreemapEntry {
    int id;
    QString name;
    qreal value;
    qreal area;
    QString category;
    int depth;
    QString parent;
    qreal percentage;
    bool leaf;
    QColor color;
};

class PaperTreemapChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperTreemapChart(QWidget* parent = nullptr);
    void addEntry(const TreemapEntry& entry);
    QList<TreemapEntry> entries() const;
    qreal totalValue() const;
    int leafCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void treemapGenerated(int id, qreal value);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawTreemapView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<TreemapEntry> entries_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
