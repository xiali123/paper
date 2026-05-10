#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct MilestoneEntry {
    int id;
    QString name;
    QString project;
    QString status;
    qreal completion;
    int daysRemaining;
    QString priority;
    QString assignee;
    QString category;
    bool onTrack;
    QColor color;
};

class PaperProjectMilestone : public QWidget {
    Q_OBJECT
public:
    explicit PaperProjectMilestone(QWidget* parent = nullptr);
    void addEntry(const MilestoneEntry& entry);
    QList<MilestoneEntry> entries() const;
    int completedCount() const;
    qreal avgCompletion() const;
    QMap<QString, int> statusCounts() const;

signals:
    void milestoneUpdated(int id, qreal completion);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawMilestoneList(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* priorityCombo_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<MilestoneEntry> entries_;
    QSettings settings_;
};
