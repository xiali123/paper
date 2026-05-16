#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct RadarEntry {
    int id;
    QString label;
    QString category;
    qreal value;
    qreal target;
    qreal maxVal;
    int axis;
    bool above;
    QColor color;
};

class PaperRadarSpinner : public QWidget {
    Q_OBJECT
public:
    explicit PaperRadarSpinner(QWidget* parent = nullptr);
    void addEntry(const RadarEntry& entry);
    QList<RadarEntry> entries() const;
    int aboveCount() const;
    qreal avgValue() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void radarGenerated(int id, qreal value);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawRadarView(QPainter& p, const QRect& rect);
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
    QList<RadarEntry> entries_;
};
