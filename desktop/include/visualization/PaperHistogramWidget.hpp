#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HistBin {
    int id; QString label; QString category; qreal minVal; qreal maxVal;
    qreal count; qreal density; bool peak; QColor color;
};
class PaperHistogramWidget : public QWidget {
    Q_OBJECT
public:
    explicit PaperHistogramWidget(QWidget* parent = nullptr);
    void addEntry(const HistBin& entry);
    QList<HistBin> entries() const;
    int peakCount() const;
    qreal maxCount() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void histGenerated(int id, qreal count);
private slots:
    void onGenerate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHistView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HistBin> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
