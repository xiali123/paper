#pragma once

#include <QWidget>
#include <QMap>
#include <QList>
#include <QPointF>

class TagCloudWidget : public QWidget {
    Q_OBJECT

public:
    explicit TagCloudWidget(QWidget* parent = nullptr);

    void setTags(const QMap<QString, int>& tagCounts);
    void clear();

signals:
    void tagClicked(const QString& tag);
    void tagSearchRequested(const QString& tag);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void layoutTags();

    struct TagItem {
        QString text;
        int count{0};
        float fontSize{12.0f};
        QRectF rect;
    };

    QList<TagItem> tags_;
    int hoveredTag_{-1};
    int maxCount_{1};
};
