#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Violin2Entry {
    int id; QString label; QString category; QString group;
    qreal median; qreal spread; bool outlier; QColor color;
};
class PaperViolinPlot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperViolinPlot2(QWidget* parent = nullptr);
    void addEntry(const Violin2Entry& entry);
    QList<Violin2Entry> entries() const;
    int outlierCount() const;
    qreal maxMedian() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pointSelected(int id, qreal median);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawViolinPlot(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Violin2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
