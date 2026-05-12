#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DendrogramEntry {
    int id; QString label; QString category; QString branch;
    qreal distance; int leaves; bool root; QColor color;
};
class PaperDendrogramPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperDendrogramPlot(QWidget* parent = nullptr);
    void addEntry(const DendrogramEntry& entry);
    QList<DendrogramEntry> entries() const;
    int rootCount() const;
    qreal maxDistance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void clusterSelected(int id, qreal distance);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDendrogram(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DendrogramEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
