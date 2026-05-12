#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HexbinEntry {
    int id; QString label; QString category; QString group;
    qreal x; qreal y; int count; bool dense; QColor color;
};
class PaperHexbinPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperHexbinPlot(QWidget* parent = nullptr);
    void addEntry(const HexbinEntry& entry);
    QList<HexbinEntry> entries() const;
    int denseCount() const;
    int maxCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void hexSelected(int id, int count);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHexbinView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HexbinEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
