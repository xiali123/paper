#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ClevelandDotEntry {
    int id; QString label; QString category; QString group;
    qreal valueA; qreal valueB; int rank; bool leader; QColor color;
};
class PaperClevelandDotPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperClevelandDotPlot(QWidget* parent = nullptr);
    void addEntry(const ClevelandDotEntry& entry);
    QList<ClevelandDotEntry> entries() const;
    int leaderCount() const;
    qreal maxDiff() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dotSelected(int id, qreal diff);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDumbbellChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ClevelandDotEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
