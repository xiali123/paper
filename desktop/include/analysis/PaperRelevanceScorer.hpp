#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ScoreEntry {
    int id; QString paper; QString category; QString metric;
    qreal score; qreal weight; qreal weighted; bool topTier; QColor color;
};
class PaperRelevanceScorer : public QWidget {
    Q_OBJECT
public:
    explicit PaperRelevanceScorer(QWidget* parent = nullptr);
    void addEntry(const ScoreEntry& entry);
    QList<ScoreEntry> entries() const;
    int topTierCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void scoreComputed(int id, qreal score);
private slots:
    void onScore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScoreList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ScoreEntry> entries_;
    QSettings settings_;
    QPushButton* scoreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
