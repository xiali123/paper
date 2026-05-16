#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct FlowSankeyEntry {
    int id; QString source; QString category; QString target;
    qreal flow; int paths; bool bidirectional; QColor color;
};
class PaperFlowSankey : public QWidget {
    Q_OBJECT
public:
    explicit PaperFlowSankey(QWidget* parent = nullptr);
    void addEntry(const FlowSankeyEntry& entry);
    QList<FlowSankeyEntry> entries() const;
    int bidirectionalCount() const;
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
    void drawSankey(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FlowSankeyEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
