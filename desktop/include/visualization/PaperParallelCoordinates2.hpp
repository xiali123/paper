#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ParallelCoordinates2Entry {
    int id; QString sample; QString category; QString dimension;
    qreal value; int axes; bool selected; QColor color;
};
class PaperParallelCoordinates2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperParallelCoordinates2(QWidget* parent = nullptr);
    void addEntry(const ParallelCoordinates2Entry& entry);
    QList<ParallelCoordinates2Entry> entries() const;
    int selectedCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sampleSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawParallelCoords(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ParallelCoordinates2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
