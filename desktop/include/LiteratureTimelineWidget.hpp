#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct TimelineEvent {
    int id{-1};
    QString title;
    QString description;
    QString category; // "publication", "milestone", "conference", "breakthrough"
    QDate date;
    QColor color;
    int impact{0}; // 1-10
};

class LiteratureTimelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit LiteratureTimelineWidget(QWidget* parent = nullptr);

    void addEvent(const TimelineEvent& event);
    QList<TimelineEvent> events() const;
    QMap<QString, int> categoryCounts() const;
    QDate earliestDate() const;
    QDate latestDate() const;

signals:
    void eventClicked(int eventId);
    void timelineUpdated(int count);

private slots:
    void onAdd();
    void onZoomChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTimeline(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawImpactChart(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* zoomCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<TimelineEvent> events_;
    int selectedEvent_{-1};
    QSettings settings_;
};
