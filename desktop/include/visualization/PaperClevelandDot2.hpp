#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ClevelandDot2Entry {
    int id; QString label; QString category; QString group;
    qreal value2024; qreal value2025; bool growth; QColor color;
};
class PaperClevelandDot2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperClevelandDot2(QWidget* parent = nullptr);
    void addEntry(const ClevelandDot2Entry& entry);
    QList<ClevelandDot2Entry> entries() const;
    int growthCount() const;
    qreal avgChange() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rowSelected(int id, qreal change);
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
    QList<ClevelandDot2Entry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
