#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TreemapChart2Entry {
    int id; QString label; QString category; QString parent;
    qreal area; int depth; bool highlight; QColor color;
};
class PaperTreemapChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperTreemapChart2(QWidget* parent = nullptr);
    void addEntry(const TreemapChart2Entry& entry);
    QList<TreemapChart2Entry> entries() const;
    int highlightCount() const;
    qreal totalArea() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void cellSelected(int id, qreal area);
private slots:
    void onLayout();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTreemap(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TreemapChart2Entry> entries_;
    QSettings settings_;
    QPushButton* layoutBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
