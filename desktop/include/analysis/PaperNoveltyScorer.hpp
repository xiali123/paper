#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct NoveltyEntry {
    int id;
    QString paperTitle;
    QString dimension;
    qreal noveltyScore;
    qreal priorWork;
    QString method;
    int uniqueIdeas;
    QString field;
    qreal significance;
    bool isNovel;
    QColor color;
};

class PaperNoveltyScorer : public QWidget {
    Q_OBJECT
public:
    explicit PaperNoveltyScorer(QWidget* parent = nullptr);
    void addEntry(const NoveltyEntry& entry);
    QList<NoveltyEntry> entries() const;
    qreal avgNovelty() const;
    int novelCount() const;
    QMap<QString, int> dimensionCounts() const;

signals:
    void noveltyScored(int id, qreal score);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onScore();
    void onClear();
    void drawScoreList(QPainter& p, const QRect& rect);
    void drawDimensionChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QPushButton* scoreBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<NoveltyEntry> entries_;
    QSettings settings_;
};
