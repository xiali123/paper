#pragma once

#include <QWidget>
#include <QLabel>
#include <QList>

class RatingWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int rating READ rating WRITE setRating NOTIFY ratingChanged)

public:
    explicit RatingWidget(QWidget* parent = nullptr);

    int rating() const { return rating_; }
    void setRating(int stars);
    void setReadOnly(bool ro);

signals:
    void ratingChanged(int stars);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int starAtPosition(int x) const;

    int rating_{0};
    int hoverRating_{0};
    bool readOnly_{false};
    static constexpr int STAR_COUNT = 5;
    static constexpr int STAR_SIZE = 20;
};
