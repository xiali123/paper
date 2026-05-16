#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingHeatmap2Entry {
    int id; QString paper; QString category; QString day;
    qreal intensity; int minutes; bool peak; QColor color;
};
class PaperReadingHeatmap2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingHeatmap2(QWidget* parent = nullptr);
    void addEntry(const ReadingHeatmap2Entry& entry);
    QList<ReadingHeatmap2Entry> entries() const;
    int peakCount() const;
    qreal totalMinutes() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void heatUpdated(int id, qreal intensity);
private slots:
    void onRefresh();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHeatmap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingHeatmap2Entry> entries_;
    QSettings settings_;
    QPushButton* refreshBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
