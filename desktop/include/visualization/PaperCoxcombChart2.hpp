#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CoxcombChart2Entry {
    int id; QString sector; QString category; QString metric;
    qreal angle; int frequency; bool dominant; QColor color;
};
class PaperCoxcombChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperCoxcombChart2(QWidget* parent = nullptr);
    void addEntry(const CoxcombChart2Entry& entry);
    QList<CoxcombChart2Entry> entries() const;
    int dominantCount() const;
    qreal maxAngle() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sectorSelected(int id, qreal angle);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCoxcomb(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CoxcombChart2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
