#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LogAggEntry {
    int id; QString source; QString category; QString level;
    qreal volume; int errors; bool alert; QColor color;
};
class PaperLogAggregator : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogAggregator(QWidget* parent = nullptr);
    void addEntry(const LogAggEntry& entry);
    QList<LogAggEntry> entries() const;
    int alertCount() const;
    qreal avgVolume() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void logAlert(int id, qreal volume);
private slots:
    void onAggregate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLogView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LogAggEntry> entries_;
    QSettings settings_;
    QPushButton* aggregateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
