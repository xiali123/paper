#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ViolinEntry {
    int id;
    QString label;
    QString category;
    qreal median;
    qreal q1, q3;
    qreal min, max;
    qreal density;
    bool skewed;
    QColor color;
};

class PaperViolinPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperViolinPlot(QWidget* parent = nullptr);
    void addEntry(const ViolinEntry& entry);
    QList<ViolinEntry> entries() const;
    int skewedCount() const;
    qreal avgMedian() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void violinGenerated(int id, qreal median);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawViolinView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ViolinEntry> entries_;
};
