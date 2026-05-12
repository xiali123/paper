#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DonutRingEntry {
    int id; QString segment; QString category; QString ring;
    qreal value; int percentage; bool outer; QColor color;
};
class PaperDonutRing : public QWidget {
    Q_OBJECT
public:
    explicit PaperDonutRing(QWidget* parent = nullptr);
    void addEntry(const DonutRingEntry& entry);
    QList<DonutRingEntry> entries() const;
    int outerCount() const;
    qreal totalValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void segmentClicked(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDonutRings(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DonutRingEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
