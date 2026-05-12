#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Scatter3Entry {
    int id; QString label; QString category; QString axis;
    qreal x; qreal y; qreal size; bool selected; QColor color;
};
class PaperScatterPlot3 : public QWidget {
    Q_OBJECT
public:
    explicit PaperScatterPlot3(QWidget* parent = nullptr);
    void addEntry(const Scatter3Entry& entry);
    QList<Scatter3Entry> entries() const;
    int selectedCount() const;
    qreal maxX() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pointClicked(int id, qreal x);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScatterView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Scatter3Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
