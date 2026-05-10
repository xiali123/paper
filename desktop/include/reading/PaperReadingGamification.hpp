#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct GamificationEntry {
    int id;
    QString userName;
    int xpPoints;
    int level;
    QString rank;
    int challengesCompleted;
    qreal completionRate;
    int streakDays;
    QString achievement;
    bool active;
    QColor color;
};

class PaperReadingGamification : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingGamification(QWidget* parent = nullptr);
    void addEntry(const GamificationEntry& entry);
    QList<GamificationEntry> entries() const;
    qreal avgXp() const;
    int activeCount() const;
    QMap<QString, int> rankCounts() const;

signals:
    void gamificationUpdated(int id, int xpPoints);

private slots:
    void onUpdate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawLeaderboard(QPainter& p, const QRect& rect);
    void drawRankChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<GamificationEntry> entries_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* periodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
