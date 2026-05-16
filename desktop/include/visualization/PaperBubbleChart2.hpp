#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BubbleEntry {
    int id; QString label; QString category; QString group;
    qreal x; qreal y; qreal size; qreal value; bool outlier; QColor color;
};
class PaperBubbleChart2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperBubbleChart2(QWidget* parent = nullptr);
    void addEntry(const BubbleEntry& entry);
    QList<BubbleEntry> entries() const;
    int outlierCount() const;
    qreal maxValue() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bubbleSelected(int id, qreal value);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBubbleView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BubbleEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
