#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TreeEntry {
    int id; QString label; QString category; QString parent;
    qreal value; qreal area; int depth; bool leaf; QColor color;
};
class PaperTreemapWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaperTreemapWidget(QWidget* parent = nullptr);
    void addEntry(const TreeEntry& entry);
    QList<TreeEntry> entries() const;
    int leafCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void treemapBuilt(int id, qreal value);
private slots:
    void onBuild();
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
    QList<TreeEntry> entries_;
    QSettings settings_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
