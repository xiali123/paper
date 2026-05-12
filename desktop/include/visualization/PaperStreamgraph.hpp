#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StreamgraphEntry {
    int id; QString stream; QString category; QString period;
    qreal value; int peaks; bool dominant; QColor color;
};
class PaperStreamgraph : public QWidget {
    Q_OBJECT
public:
    explicit PaperStreamgraph(QWidget* parent = nullptr);
    void addEntry(const StreamgraphEntry& entry);
    QList<StreamgraphEntry> entries() const;
    int dominantCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void streamSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawStreamgraph(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StreamgraphEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
