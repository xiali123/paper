#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct StreakBoardEntry {
    int id;
    QString readerName;
    int currentStreak;
    int bestStreak;
    int totalPapers;
    QString badge;
    int rank;
    qreal avgPerDay;
    QString period;
    bool active;
    QColor color;
};

class PaperReadingStreakBoard : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingStreakBoard(QWidget* parent = nullptr);
    void addEntry(const StreakBoardEntry& entry);
    QList<StreakBoardEntry> entries() const;
    int activeCount() const;
    qreal avgStreak() const;
    QMap<QString, int> periodCounts() const;
signals:
    void streakUpdated(int id, int currentStreak);
private slots:
    void onUpdate();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawStreakList(QPainter& p, const QRect& rect);
    void drawPeriodChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StreakBoardEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* periodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
