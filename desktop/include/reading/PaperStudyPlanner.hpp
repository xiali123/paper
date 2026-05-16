#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct StudyEntry {
    int id;
    QString topic;
    QString category;
    QString priority;
    int duration;
    int progress;
    QString day;
    bool completed;
    QColor color;
};

class PaperStudyPlanner : public QWidget {
    Q_OBJECT
public:
    explicit PaperStudyPlanner(QWidget* parent = nullptr);
    void addEntry(const StudyEntry& entry);
    QList<StudyEntry> entries() const;
    int completedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void studyPlanned(int id, int progress);

private slots:
    void onPlan();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawStudyList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<StudyEntry> entries_;
    QSettings settings_;
    QPushButton* planBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
