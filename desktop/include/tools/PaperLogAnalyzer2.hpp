#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LogEntry {
    int id; QString source; QString category; QString level;
    qreal frequency; int occurrences; bool critical; QColor color;
};
class PaperLogAnalyzer2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogAnalyzer2(QWidget* parent = nullptr);
    void addEntry(const LogEntry& entry);
    QList<LogEntry> entries() const;
    int criticalCount() const;
    qreal avgFrequency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void patternFound(int id, qreal frequency);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLogChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LogEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
