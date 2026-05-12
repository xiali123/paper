#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SankeyEntry {
    int id; QString source; QString target; QString category;
    qreal value; qreal flow; qreal efficiency; bool mainFlow; QColor color;
};
class PaperSankeyChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperSankeyChart(QWidget* parent = nullptr);
    void addEntry(const SankeyEntry& entry);
    QList<SankeyEntry> entries() const;
    int mainFlowCount() const;
    qreal totalFlow() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sankeyRendered(int id, qreal flow);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSankeyView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SankeyEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
