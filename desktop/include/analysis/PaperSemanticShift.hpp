#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SemanticShiftEntry {
    int id; QString term; QString category; QString epoch;
    qreal shiftScore; int occurrences; bool significant; QColor color;
};
class PaperSemanticShift : public QWidget {
    Q_OBJECT
public:
    explicit PaperSemanticShift(QWidget* parent = nullptr);
    void addEntry(const SemanticShiftEntry& entry);
    QList<SemanticShiftEntry> entries() const;
    int significantCount() const;
    qreal avgShiftScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void shiftDetected(int id, qreal score);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawShiftTimeline(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SemanticShiftEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
