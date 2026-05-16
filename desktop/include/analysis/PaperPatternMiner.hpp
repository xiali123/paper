#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PatternEntry {
    int id; QString pattern; QString category; QString frequency;
    qreal support; qreal confidence; int occurrences; bool frequent; QColor color;
};
class PaperPatternMiner : public QWidget {
    Q_OBJECT
public:
    explicit PaperPatternMiner(QWidget* parent = nullptr);
    void addEntry(const PatternEntry& entry);
    QList<PatternEntry> entries() const;
    int frequentCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void patternMined(int id, qreal confidence);
private slots:
    void onMine();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPatternList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PatternEntry> entries_;
    QSettings settings_;
    QPushButton* mineBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
