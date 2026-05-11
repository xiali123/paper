#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct TimelineEntry {
    int id;
    QString event;
    QString category;
    QString date;
    QString milestone;
    int impact;
    bool keyEvent;
    QColor color;
};

class PaperPaperTimeline : public QWidget {
    Q_OBJECT
public:
    explicit PaperPaperTimeline(QWidget* parent = nullptr);
    void addEntry(const TimelineEntry& entry);
    QList<TimelineEntry> entries() const;
    int keyEventCount() const;
    qreal avgImpact() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void timelineCreated(int id, int impact);
private slots:
    void onCreate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTimelineList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TimelineEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
