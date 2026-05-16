#include "paper/RatingWidget.hpp"
#include <QPainter>
#include <QMouseEvent>

RatingWidget::RatingWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(STAR_COUNT * (STAR_SIZE + 4) + 4, STAR_SIZE + 8);
    setCursor(Qt::PointingHandCursor);
}

void RatingWidget::setRating(int stars) {
    rating_ = qBound(0, stars, STAR_COUNT);
    update();
    emit ratingChanged(rating_);
}

void RatingWidget::setReadOnly(bool ro) {
    readOnly_ = ro;
    setCursor(ro ? Qt::ArrowCursor : Qt::PointingHandCursor);
}

void RatingWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int display = hoverRating_ > 0 ? hoverRating_ : rating_;

    for (int i = 0; i < STAR_COUNT; ++i) {
        int x = 2 + i * (STAR_SIZE + 4);
        int y = 4;

        // Star polygon
        QPolygonF star;
        float cx = x + STAR_SIZE / 2.0f;
        float cy = y + STAR_SIZE / 2.0f;
        float outerR = STAR_SIZE / 2.0f;
        float innerR = outerR * 0.4f;

        for (int j = 0; j < 5; ++j) {
            double angle = (j * 72.0 - 90.0) * M_PI / 180.0;
            star << QPointF(cx + outerR * cos(angle), cy + outerR * sin(angle));
            angle = ((j * 72.0 + 36.0) - 90.0) * M_PI / 180.0;
            star << QPointF(cx + innerR * cos(angle), cy + innerR * sin(angle));
        }

        if (i < display) {
            painter.setBrush(QColor(234, 179, 8));
            painter.setPen(QColor(202, 138, 4));
        } else {
            painter.setBrush(palette().base().color());
            painter.setPen(palette().mid().color());
        }

        painter.drawPolygon(star);
    }
}

void RatingWidget::mousePressEvent(QMouseEvent* event) {
    if (readOnly_) return;
    int star = starAtPosition(event->pos().x());
    if (star >= 0 && star <= STAR_COUNT) {
        setRating(star);
    }
}

void RatingWidget::mouseMoveEvent(QMouseEvent* event) {
    if (readOnly_) return;
    int star = starAtPosition(event->pos().x());
    if (star != hoverRating_) {
        hoverRating_ = star;
        update();
    }
}

void RatingWidget::leaveEvent(QEvent*) {
    hoverRating_ = 0;
    update();
}

int RatingWidget::starAtPosition(int x) const {
    for (int i = 0; i < STAR_COUNT; ++i) {
        int starX = 2 + i * (STAR_SIZE + 4);
        if (x >= starX && x <= starX + STAR_SIZE) {
            return i + 1;
        }
    }
    return 0;
}
