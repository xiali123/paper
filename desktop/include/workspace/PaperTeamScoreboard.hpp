#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct ScoreEntry {
    int id;
    QString member;
    QString category;
    QString metric;
    qreal score;
    int rank;
    QString period;
    bool leader;
    QColor color;
};

class PaperTeamScoreboard : public QWidget {
    Q_OBJECT
public:
    explicit PaperTeamScoreboard(QWidget* parent = nullptr);
    void addEntry(const ScoreEntry& entry);
    QList<ScoreEntry> entries() const;
    int leaderCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void scoreUpdated(int id, qreal score);

private slots:
    void onAdd();
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
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
