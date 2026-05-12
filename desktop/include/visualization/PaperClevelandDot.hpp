#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ClevelandDotEntry {
    int id; QString label; QString category; QString metric;
    qreal actual; qreal target; bool exceeded; QColor color;
};
class PaperClevelandDot : public QWidget {
    Q_OBJECT
public:
    explicit PaperClevelandDot(QWidget* parent = nullptr);
    void addEntry(const ClevelandDotEntry& entry);
    QList<ClevelandDotEntry> entries() const;
    int exceededCount() const;
    qreal avgActual() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void dotClicked(int id, qreal actual);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDotPlot(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ClevelandDotEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
