#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct CoReadingSession {
    int id{-1};
    QString paperTitle;
    QString reader;
    QString role; // "leader", "member", "observer"
    QDate date;
    int durationMin{0};
    int pagesRead{0};
    QString notes;
    QColor color;
};

class PaperCoReadingTracker : public QWidget {
    Q_OBJECT

public:
    explicit PaperCoReadingTracker(QWidget* parent = nullptr);

    void addSession(const CoReadingSession& session);
    QList<CoReadingSession> sessions() const;
    qreal totalHours() const;
    int totalSessions() const;
    QMap<QString, int> readerCounts() const;

signals:
    void sessionClicked(int sessionId);
    void statsUpdated(int sessions, qreal hours);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTimeline(QPainter& p, const QRect& rect);
    void drawReaderChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<CoReadingSession> sessions_;
    QSettings settings_;
};
