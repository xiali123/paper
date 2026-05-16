#pragma once

#include <QWidget>
#include <QList>
#include <QMap>

struct Paper;

class PaperTimelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperTimelineWidget(QWidget* parent = nullptr);

    void setPapers(const QList<Paper>& papers);
    void clear();

signals:
    void paperClicked(int paperId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    struct TimelineEntry {
        int paperId{0};
        QString title;
        QString authors;
        QString year;
        int citations{0};
        int yOffset{0};
    };

    void buildTimeline();

    QList<TimelineEntry> entries_;
    QMap<int, int> yearCounts_;
    int minYear_{9999};
    int maxYear_{0};
    int hoveredEntry_{-1};
};
