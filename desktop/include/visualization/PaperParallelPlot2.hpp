#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ParallelEntry {
    int id; QString label; QString category; QString axis;
    qreal v1; qreal v2; qreal v3; bool highlight; QColor color;
};
class PaperParallelPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperParallelPlot2(QWidget* parent = nullptr);
    void addEntry(const ParallelEntry& entry);
    QList<ParallelEntry> entries() const;
    int highlightCount() const;
    qreal maxV1() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void lineSelected(int id, qreal v1);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawParallelView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ParallelEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
