#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct ChallengeEntry {
    int id{-1};
    QString name;
    QString type; // "daily", "weekly", "monthly", "custom"
    int target{0};
    int current{0};
    QDate startDate;
    QDate endDate;
    QString reward;
    bool completed{false};
    QColor color;
};

class PaperReadingChallengeWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperReadingChallengeWidget(QWidget* parent = nullptr);

    void addChallenge(const ChallengeEntry& challenge);
    QList<ChallengeEntry> challenges() const;
    QMap<QString, int> typeCounts() const;
    int activeChallenges() const;
    int completedChallenges() const;

signals:
    void challengeStarted(int id);
    void challengeCompleted(int id);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawChallengeCards(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ChallengeEntry> challenges_;
    QSettings settings_;
};
