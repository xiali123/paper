#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct CitationTimingEntry {
    int id;
    QString paperTitle;
    int citationCount;
    int yearsSincePub;
    qreal citationsPerYear;
    QString trend;
    qreal acceleration;
    int peakYear;
    QString color;
    QColor barColor;
};

class PaperCitationTimingAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationTimingAnalyzer(QWidget* parent = nullptr);
    void addEntry(const CitationTimingEntry& entry);
    QList<CitationTimingEntry> entries() const;
    QMap<QString, int> trendCounts() const;
    qreal avgCitationsPerYear() const;
    int totalCitations() const;

signals:
    void timingAnalyzed(int id, const QString& trend);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAnalyze();
    void onClear();
    void drawTimingList(QPainter& p, const QRect& rect);
    void drawTrendChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<CitationTimingEntry> entries_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
