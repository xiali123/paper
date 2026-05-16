#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CoxcombEntry {
    int id; QString label; QString category; QString wedge;
    qreal value; qreal angle; bool peak; QColor color;
};
class PaperCoxcombChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperCoxcombChart(QWidget* parent = nullptr);
    void addEntry(const CoxcombEntry& entry);
    QList<CoxcombEntry> entries() const;
    int peakCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void wedgeSelected(int id, qreal value);
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
    QList<CoxcombEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
