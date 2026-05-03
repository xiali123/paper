#pragma once

#include <QWidget>
#include <QPainter>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>

struct TimelineEntry {
    int paperId{-1};
    QString title;
    int year{0};
    int month{0};
    QString category;
    QString description;
    QColor color{QColor(59, 130, 246)};
};

class PaperTimelineBuilder : public QWidget {
    Q_OBJECT

public:
    explicit PaperTimelineBuilder(QWidget* parent = nullptr);

    void addEntry(const TimelineEntry& entry);
    void addEntries(const QList<TimelineEntry>& entries);
    void clearTimeline();
    QList<TimelineEntry> entries() const;
    void setGroupBy(const QString& field);

signals:
    void entryClicked(int paperId);
    void entryDoubleClicked(int paperId);
    void timelineExported();

private slots:
    void onGroupChanged(int index);
    void onExport();
    void onItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void refreshList();
    void rebuildTimeline();

    QListWidget* entryList_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QComboBox* groupCombo_{nullptr};

    QList<TimelineEntry> entries_;
    QString groupField_{"year"};
};
