#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HeaderEntry {
    int id; QString header; QString category; QString protocol;
    qreal latency; int size; bool compressed; QColor color;
};
class PaperHeaderAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperHeaderAnalyzer(QWidget* parent = nullptr);
    void addEntry(const HeaderEntry& entry);
    QList<HeaderEntry> entries() const;
    int compressedCount() const;
    qreal avgLatency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void headerParsed(int id, qreal latency);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHeaderView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HeaderEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
