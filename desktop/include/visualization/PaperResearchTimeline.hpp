#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct ResearchEvent {
    int id{-1};
    QString title;
    QString eventType; // "publication", "review", "conference", "milestone"
    QDate date;
    QString description;
    QColor color;
};

class PaperResearchTimeline : public QWidget {
    Q_OBJECT

public:
    explicit PaperResearchTimeline(QWidget* parent = nullptr);

    void addEvent(const ResearchEvent& event);
    QList<ResearchEvent> events() const;
    QMap<QString, int> typeCounts() const;
    int eventsThisMonth() const;

signals:
    void eventClicked(int eventId);
    void timelineUpdated(int count);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTimelineTrack(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ResearchEvent> events_;
    int selectedEvent_{-1};
    QSettings settings_;
};
