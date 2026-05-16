#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DendroEntry {
    int id; QString cluster; QString category; QString method;
    qreal height; qreal distance; int members; bool leaf; QColor color;
};
class PaperDendrogramView : public QWidget {
    Q_OBJECT
public:
    explicit PaperDendrogramView(QWidget* parent = nullptr);
    void addEntry(const DendroEntry& entry);
    QList<DendroEntry> entries() const;
    int leafCount() const;
    qreal maxHeight() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dendroBuilt(int id, qreal height);
private slots:
    void onBuild();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDendroView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DendroEntry> entries_;
    QSettings settings_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
