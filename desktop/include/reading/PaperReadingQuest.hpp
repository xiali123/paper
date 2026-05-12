#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct QuestEntry {
    int id; QString mission; QString category; QString difficulty;
    qreal completion; int papersRead; bool achieved; QColor color;
};
class PaperReadingQuest : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingQuest(QWidget* parent = nullptr);
    void addEntry(const QuestEntry& entry);
    QList<QuestEntry> entries() const;
    int achievedCount() const;
    qreal avgCompletion() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void questComplete(int id, qreal completion);
private slots:
    void onStart();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawQuestBoard(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<QuestEntry> entries_;
    QSettings settings_;
    QPushButton* startBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
