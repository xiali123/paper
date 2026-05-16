#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct SprintTask {
    int id;
    QString taskName;
    QString assignee;
    QString status;
    QString priority;
    int storyPoints;
    int progress;
    QString sprint;
    bool blocked;
    QColor color;
};

class PaperSprintBoard : public QWidget {
    Q_OBJECT
public:
    explicit PaperSprintBoard(QWidget* parent = nullptr);
    void addEntry(const SprintTask& entry);
    QList<SprintTask> entries() const;
    int completedPoints() const;
    int blockedCount() const;
    QMap<QString, int> statusCounts() const;

signals:
    void sprintUpdated(int id, int progress);

private slots:
    void onAdd();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawSprintView(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* sprintCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<SprintTask> entries_;
};
