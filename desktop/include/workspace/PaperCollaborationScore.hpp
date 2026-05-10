#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct CollabScoreEntry {
    int id;
    QString authorA;
    QString authorB;
    qreal score;
    QString dimension;
    int jointPapers;
    qreal similarity;
    QString institution;
    QString field;
    QColor color;
};

class PaperCollaborationScore : public QWidget {
    Q_OBJECT
public:
    explicit PaperCollaborationScore(QWidget* parent = nullptr);
    void addEntry(const CollabScoreEntry& entry);
    QList<CollabScoreEntry> entries() const;
    qreal avgScore() const;
    int topPairs() const;
    QMap<QString, int> dimensionCounts() const;

signals:
    void scoreComputed(int id, qreal score);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onCompute();
    void onClear();
    void drawPairList(QPainter& p, const QRect& rect);
    void drawDimensionChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QPushButton* computeBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<CollabScoreEntry> entries_;
    QSettings settings_;
};
