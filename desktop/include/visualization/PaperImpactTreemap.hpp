#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct TreemapEntry {
    int id;
    QString field;
    int paperCount;
    qreal impact;
    qreal size;
    QString trend;
    int citations;
    QString topPaper;
    qreal growth;
    QColor color;
};

class PaperImpactTreemap : public QWidget {
    Q_OBJECT
public:
    explicit PaperImpactTreemap(QWidget* parent = nullptr);
    void addEntry(const TreemapEntry& entry);
    QList<TreemapEntry> entries() const;
    qreal totalImpact() const;
    int topFields() const;
    QMap<QString, int> trendCounts() const;

signals:
    void treemapGenerated(int id, qreal impact);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawTreemapView(QPainter& p, const QRect& rect);
    void drawTrendChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<TreemapEntry> entries_;
    QSettings settings_;
};
