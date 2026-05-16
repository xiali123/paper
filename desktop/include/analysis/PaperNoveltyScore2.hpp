#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct NoveltyScore2Entry {
    int id; QString paper; QString category; QString metric;
    qreal score; int citations; bool novel; QColor color;
};
class PaperNoveltyScore2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperNoveltyScore2(QWidget* parent = nullptr);
    void addEntry(const NoveltyScore2Entry& entry);
    QList<NoveltyScore2Entry> entries() const;
    int novelCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void scoreCalculated(int id, qreal score);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScoreView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<NoveltyScore2Entry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
