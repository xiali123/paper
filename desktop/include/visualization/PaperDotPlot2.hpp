#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DotPlot2Entry {
    int id; QString label; QString category; QString axis;
    qreal value; int count; bool highlighted; QColor color;
};
class PaperDotPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperDotPlot2(QWidget* parent = nullptr);
    void addEntry(const DotPlot2Entry& entry);
    QList<DotPlot2Entry> entries() const;
    int highlightedCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dotSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDotPlot(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DotPlot2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
