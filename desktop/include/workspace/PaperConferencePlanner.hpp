#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct ConferenceEntry {
    int id{-1};
    QString name;
    QString venue;
    QDate deadline;
    QDate notificationDate;
    QDate conferenceDate;
    QString status; // "planning", "submitted", "accepted", "rejected", "presented"
    QString track;
    qreal registrationFee{0};
    QColor color;
};

class PaperConferencePlanner : public QWidget {
    Q_OBJECT

public:
    explicit PaperConferencePlanner(QWidget* parent = nullptr);

    void addConference(const ConferenceEntry& conference);
    QList<ConferenceEntry> conferences() const;
    QMap<QString, int> statusCounts() const;
    int upcomingCount() const;
    qreal totalFees() const;

signals:
    void deadlineApproaching(int confId, int daysLeft);
    void conferenceAdded(int id);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTimeline(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ConferenceEntry> conferences_;
    QSettings settings_;
};
