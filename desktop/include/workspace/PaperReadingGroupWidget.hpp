#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>

struct ReadingGroup {
    int id{-1};
    QString name;
    QStringList members;
    QString paperTitle;
    QString status; // "active", "paused", "completed"
    int progress{0}; // 0-100
    QDate startDate;
    QDate nextMeeting;
    QColor color;
    QString notes;
};

class PaperReadingGroupWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperReadingGroupWidget(QWidget* parent = nullptr);

    void addGroup(const ReadingGroup& group);
    QList<ReadingGroup> groups() const;
    QMap<QString, int> statusCounts() const;
    int activeGroups() const;

signals:
    void groupClicked(int groupId);
    void groupsUpdated(int count);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawGroupCards(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ReadingGroup> groups_;
    int selectedGroup_{-1};
    QSettings settings_;
};
