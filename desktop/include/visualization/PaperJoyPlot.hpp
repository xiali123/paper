#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct JoyEntry {
    int id; QString series; QString category; QString group;
    qreal value; qreal peak; qreal baseline; bool dominant; QColor color;
};
class PaperJoyPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperJoyPlot(QWidget* parent = nullptr);
    void addEntry(const JoyEntry& entry);
    QList<JoyEntry> entries() const;
    int dominantCount() const;
    qreal maxPeak() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void joyRendered(int id, qreal peak);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawJoyView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<JoyEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
