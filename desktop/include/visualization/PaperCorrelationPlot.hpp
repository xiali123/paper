#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CorrEntry {
    int id; QString varX; QString category; QString varY;
    qreal coefficient; int samples; bool significant; QColor color;
};
class PaperCorrelationPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperCorrelationPlot(QWidget* parent = nullptr);
    void addEntry(const CorrEntry& entry);
    QList<CorrEntry> entries() const;
    int significantCount() const;
    qreal maxCoefficient() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pairSelected(int id, qreal coefficient);
private slots:
    void onRender();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCorrMatrix(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CorrEntry> entries_;
    QSettings settings_;
    QPushButton* renderBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
