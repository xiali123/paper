#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct GaugeEntry {
    int id;
    QString metric;
    qreal value;
    qreal target;
    qreal maxVal;
    QString unit;
    QString category;
    QString status;
    int rank;
    bool onTarget;
    QColor color;
};

class PaperGaugeChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperGaugeChart(QWidget* parent = nullptr);
    void addEntry(const GaugeEntry& entry);
    QList<GaugeEntry> entries() const;
    qreal avgValue() const;
    int onTargetCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void gaugeGenerated(int id, qreal value);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawGaugeView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<GaugeEntry> entries_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
