#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BeeswarmChart2Entry {
    int id; QString point; QString category; QString axis;
    qreal value; int cluster; bool highlighted; QColor color;
};
class PaperBeeswarmChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperBeeswarmChart2(QWidget* parent = nullptr);
    void addEntry(const BeeswarmChart2Entry& entry);
    QList<BeeswarmChart2Entry> entries() const;
    int highlightedCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pointSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBeeswarm(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BeeswarmChart2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
