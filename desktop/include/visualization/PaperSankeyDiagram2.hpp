#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SankeyEntry {
    int id; QString source; QString category; QString target;
    qreal flow; qreal width; int connections; bool dominant; QColor color;
};
class PaperSankeyDiagram2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperSankeyDiagram2(QWidget* parent = nullptr);
    void addEntry(const SankeyEntry& entry);
    QList<SankeyEntry> entries() const;
    int dominantCount() const;
    qreal totalFlow() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void flowSelected(int id, qreal flow);
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
