#include "visualization/TagCloudWidget.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

TagCloudWidget::TagCloudWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setMouseTracking(true);
}

void TagCloudWidget::setTags(const QMap<QString, int>& tagCounts) {
    tags_.clear();
    maxCount_ = 1;

    for (auto it = tagCounts.constBegin(); it != tagCounts.constEnd(); ++it) {
        TagItem item;
        item.text = it.key();
        item.count = it.value();
        tags_.append(item);
        maxCount_ = qMax(maxCount_, it.value());
    }

    std::sort(tags_.begin(), tags_.end(),
        [](const TagItem& a, const TagItem& b) { return a.count > b.count; });

    if (tags_.size() > 50) tags_ = tags_.mid(0, 50);

    layoutTags();
    update();
}

void TagCloudWidget::clear() {
    tags_.clear();
    update();
}

void TagCloudWidget::layoutTags() {
    if (tags_.isEmpty()) return;

    QFontMetrics fm(font());
    int x = 8;
    int y = 8;
    int lineHeight = 0;
    int widgetWidth = width() > 100 ? width() : 400;

    for (auto& tag : tags_) {
        float ratio = (float)tag.count / maxCount_;
        tag.fontSize = 10.0f + ratio * 18.0f;

        QFont tagFont(font().family(), (int)tag.fontSize);
        QFontMetrics tagFm(tagFont);
        int textWidth = tagFm.horizontalAdvance(tag.text) + 16;
        int textHeight = tagFm.height() + 8;

        if (x + textWidth > widgetWidth - 8) {
            x = 8;
            y += lineHeight + 4;
            lineHeight = 0;
        }

        tag.rect = QRectF(x, y, textWidth, textHeight);
        lineHeight = qMax(lineHeight, textHeight);
        x += textWidth + 6;
    }

    int totalHeight = y + lineHeight + 16;
    setMinimumHeight(totalHeight);
}

void TagCloudWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().window());

    for (int i = 0; i < tags_.size(); ++i) {
        const auto& tag = tags_[i];
        bool hovered = (i == hoveredTag_);

        float ratio = (float)tag.count / maxCount_;

        QColor bg;
        if (hovered) {
            bg = QColor(59, 130, 246);
        } else {
            int lightness = 200 + (1.0 - ratio) * 55;
            bg = QColor(lightness, lightness, lightness + 20);
            if (palette().window().color().lightness() < 128) {
                bg = QColor(60 + ratio * 40, 70 + ratio * 40, 100 + ratio * 60);
            }
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(bg);
        painter.drawRoundedRect(tag.rect, 12, 12);

        QColor textColor = hovered ? Qt::white : palette().text().color();
        painter.setPen(textColor);
        QFont tagFont(font().family(), (int)tag.fontSize);
        painter.setFont(tagFont);
        painter.drawText(tag.rect, Qt::AlignCenter, tag.text);
    }
}

void TagCloudWidget::mousePressEvent(QMouseEvent* event) {
    for (int i = 0; i < tags_.size(); ++i) {
        if (tags_[i].rect.contains(event->pos())) {
            emit tagClicked(tags_[i].text);
            emit tagSearchRequested(tags_[i].text);
            return;
        }
    }
}

void TagCloudWidget::mouseMoveEvent(QMouseEvent* event) {
    int oldHovered = hoveredTag_;
    hoveredTag_ = -1;
    for (int i = 0; i < tags_.size(); ++i) {
        if (tags_[i].rect.contains(event->pos())) {
            hoveredTag_ = i;
            break;
        }
    }
    if (hoveredTag_ != oldHovered) update();
    setCursor(hoveredTag_ >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
}
