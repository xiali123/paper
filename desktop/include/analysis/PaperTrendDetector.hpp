#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TrendEntry {
    int id; QString topic; QString category; QString direction;
    qreal strength; qreal velocity; qreal confidence; bool emerging; QColor color;
};
class PaperTrendDetector : public QWidget {
    Q_OBJECT
public:
    explicit PaperTrendDetector(QWidget* parent = nullptr);
    void addEntry(const TrendEntry& entry);
    QList<TrendEntry> entries() const;
    int emergingCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void trendDetected(int id, qreal strength);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTrendList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TrendEntry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
