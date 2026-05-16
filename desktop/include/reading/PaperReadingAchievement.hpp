#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct AchievementEntry {
    int id;
    QString userName;
    QString achievement;
    QString tier;
    int points;
    QString category;
    int papersRequired;
    int papersDone;
    qreal progress;
    QString date;
    bool unlocked;
    QColor color;
};

class PaperReadingAchievement : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingAchievement(QWidget* parent = nullptr);
    void addEntry(const AchievementEntry& entry);
    QList<AchievementEntry> entries() const;
    int unlockedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> tierCounts() const;

signals:
    void achievementUnlocked(int id, int points);

private slots:
    void onUnlock();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawAchievementList(QPainter& p, const QRect& rect);
    void drawTierChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<AchievementEntry> entries_;
    QPushButton* unlockBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
