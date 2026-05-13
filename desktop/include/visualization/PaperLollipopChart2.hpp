#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LollipopChart2Entry {
    int id; QString label; QString category; QString metric;
    qreal value; int rank; bool highlighted; QColor color;
};
class PaperLollipopChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperLollipopChart2(QWidget* parent = nullptr);
    void addEntry(const LollipopChart2Entry& entry);
    QList<LollipopChart2Entry> entries() const;
    int highlightedCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void lollipopSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLollipopChart(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LollipopChart2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
