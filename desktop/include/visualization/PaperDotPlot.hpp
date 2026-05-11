#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct DotEntry {
    int id;
    QString label;
    QString group;
    QString category;
    qreal x;
    qreal y;
    qreal size;
    bool outlier;
    QColor color;
};

class PaperDotPlot : public QWidget {
    Q_OBJECT
public:
    explicit PaperDotPlot(QWidget* parent = nullptr);
    void addEntry(const DotEntry& entry);
    QList<DotEntry> entries() const;
    int outlierCount() const;
    QMap<QString, int> groupCounts() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void dotPlotted(int id, qreal x);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawDotView(QPainter& p, const QRect& rect);
    void drawGroupLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<DotEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
