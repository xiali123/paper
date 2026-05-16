#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct ReadingChallengeEntry {
    int id;
    QString challengeName;
    QString difficulty;
    int targetPapers;
    int completedPapers;
    qreal progress;
    QString duration;
    int participants;
    QString badge;
    bool completed;
    QColor color;
};

class PaperReadingChallenge : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingChallenge(QWidget* parent = nullptr);
    void addEntry(const ReadingChallengeEntry& entry);
    QList<ReadingChallengeEntry> entries() const;
    int completedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> difficultyCounts() const;
signals:
    void challengeJoined(int id, qreal progress);
private slots:
    void onJoin();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawChallengeList(QPainter& p, const QRect& rect);
    void drawDifficultyChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingChallengeEntry> entries_;
    QSettings settings_;
    QPushButton* joinBtn_;
    QPushButton* clearBtn_;
    QComboBox* difficultyCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
