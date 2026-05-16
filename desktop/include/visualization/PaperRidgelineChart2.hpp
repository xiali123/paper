#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct Ridge2Entry {
    int id; QString label; QString category; QString group;
    qreal peak; qreal spread; bool outlier; QColor color;
};
class PaperRidgelineChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperRidgelineChart2(QWidget* parent = nullptr);
    void addEntry(const Ridge2Entry& entry);
    QList<Ridge2Entry> entries() const;
    int outlierCount() const;
    qreal maxPeak() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void ridgeSelected(int id, qreal peak);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRidgeline(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<Ridge2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
