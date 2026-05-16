#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct RepetitionEntry {
    int id;
    QString topic;
    QString category;
    QString difficulty;
    int interval;
    int repetitions;
    qreal easeFactor;
    QString nextReview;
    bool due;
    QColor color;
};

class PaperSpacedRepetition : public QWidget {
    Q_OBJECT
public:
    explicit PaperSpacedRepetition(QWidget* parent = nullptr);
    void addEntry(const RepetitionEntry& entry);
    QList<RepetitionEntry> entries() const;
    int dueCount() const;
    qreal avgEase() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void reviewScheduled(int id, const QString& nextReview);

private slots:
    void onSchedule();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawScheduleList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<RepetitionEntry> entries_;
    QSettings settings_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
