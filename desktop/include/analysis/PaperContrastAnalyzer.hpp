#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ContrastEntry {
    int id; QString paperA; QString paperB; QString category; QString dimension;
    qreal similarity; qreal difference; qreal significance; bool divergent; QColor color;
};
class PaperContrastAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperContrastAnalyzer(QWidget* parent = nullptr);
    void addEntry(const ContrastEntry& entry);
    QList<ContrastEntry> entries() const;
    int divergentCount() const;
    qreal avgDifference() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void contrastAnalyzed(int id, qreal difference);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawContrastList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ContrastEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
