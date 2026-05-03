#include "PaperTimelineWidget.hpp"
#include "PaperTypes.hpp"
#include <QPainter>
#include <QMouseEvent>
#include <QDate>

PaperTimelineWidget::PaperTimelineWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
}

void PaperTimelineWidget::setPapers(const QList<Paper>& papers) {
    entries_.clear();
    yearCounts_.clear();
    minYear_ = 9999;
    maxYear_ = 0;

    for (const auto& p : papers) {
        bool ok;
        int year = p.year.toInt(&ok);
        if (!ok || year < 1900 || year > 2100) continue;

        TimelineEntry entry;
        entry.paperId = p.id;
        entry.title = p.title;
        entry.authors = p.authors;
        entry.year = p.year;
        entry.citations = p.citationCount;
        entries_.append(entry);

        yearCounts_[year]++;
        minYear_ = qMin(minYear_, year);
        maxYear_ = qMax(maxYear_, year);
    }

    if (minYear_ > maxYear_) { minYear_ = 2020; maxYear_ = 2026; }

    buildTimeline();
    update();
}

void PaperTimelineWidget::clear() {
    entries_.clear();
    yearCounts_.clear();
    update();
}

void PaperTimelineWidget::buildTimeline() {
    // Sort by year
    std::sort(entries_.begin(), entries_.end(),
        [](const TimelineEntry& a, const TimelineEntry& b) {
            return a.year < b.year;
        });

    int y = 0;
    QMap<int, int> yearSlot;
    for (auto& entry : entries_) {
        int year = entry.year.toInt();
        int slot = yearSlot[year]++;
        entry.yOffset = slot * 60;
        y = qMax(y, entry.yOffset);
    }
    setMinimumHeight(qMax(200, y + 120));
}

void PaperTimelineWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().window());

    if (entries_.isEmpty()) {
        painter.setPen(palette().mid().color());
        painter.setFont(QFont("Arial", 12));
        painter.drawText(rect(), Qt::AlignCenter, "No papers to display");
        return;
    }

    int margin = 60;
    int timelineY = 40;
    int width = this->width() - 2 * margin;
    int yearSpan = qMax(1, maxYear_ - minYear_);

    // Timeline axis
    painter.setPen(QPen(palette().mid().color(), 2));
    painter.drawLine(margin, timelineY, width + margin, timelineY);

    // Year markers
    painter.setFont(QFont("Arial", 9));
    painter.setPen(palette().mid().color());
    for (int year = minYear_; year <= maxYear_; ++year) {
        float x = margin + ((year - minYear_) / (float)yearSpan) * width;
        painter.drawLine(x, timelineY - 4, x, timelineY + 4);
        painter.drawText(QRectF(x - 25, timelineY + 6, 50, 16), Qt::AlignCenter, QString::number(year));

        // Year count badge
        int count = yearCounts_.value(year, 0);
        if (count > 0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(59, 130, 246));
            painter.drawEllipse(QPointF(x, timelineY - 16), 10, 10);
            painter.setPen(Qt::white);
            painter.setFont(QFont("Arial", 7, QFont::Bold));
            painter.drawText(QRectF(x - 10, timelineY - 22, 20, 14), Qt::AlignCenter, QString::number(count));
            painter.setPen(palette().mid().color());
        }
    }

    // Paper entries
    int entryY = timelineY + 40;
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        int year = entry.year.toInt();
        float x = margin + ((year - minYear_) / (float)yearSpan) * width;
        int y = entryY + entry.yOffset;

        // Connector line
        painter.setPen(QPen(QColor(200, 200, 200), 1, Qt::DashLine));
        painter.drawLine(x, timelineY + 4, x, y);

        // Entry card
        bool hovered = (i == hoveredEntry_);
        QColor cardBg = hovered ? QColor(59, 130, 246) : palette().base().color();
        QColor cardText = hovered ? Qt::white : palette().text().color();

        painter.setPen(Qt::NoPen);
        painter.setBrush(cardBg);
        painter.drawRoundedRect(QRectF(x - 80, y, 160, 50), 6, 6);

        painter.setPen(cardText);
        painter.setFont(QFont("Arial", 9));
        QString title = entry.title;
        if (title.length() > 25) title = title.left(23) + "...";
        painter.drawText(QRectF(x - 75, y + 4, 150, 18), Qt::AlignLeft, title);

        painter.setFont(QFont("Arial", 8));
        painter.setPen(hovered ? QColor(200, 220, 255) : palette().mid().color());
        QString authors = entry.authors;
        if (authors.length() > 25) authors = authors.left(23) + "...";
        painter.drawText(QRectF(x - 75, y + 22, 150, 14), Qt::AlignLeft, authors);

        if (entry.citations > 0) {
            painter.drawText(QRectF(x - 75, y + 36, 150, 12), Qt::AlignLeft,
                QString("Cited: %1").arg(entry.citations));
        }
    }
}

void PaperTimelineWidget::mousePressEvent(QMouseEvent* event) {
    int margin = 60;
    int timelineY = 40;
    int width = this->width() - 2 * margin;
    int yearSpan = qMax(1, maxYear_ - minYear_);
    int entryY = timelineY + 40;

    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        int year = entry.year.toInt();
        float x = margin + ((year - minYear_) / (float)yearSpan) * width;
        int y = entryY + entry.yOffset;

        QRectF card(x - 80, y, 160, 50);
        if (card.contains(event->pos())) {
            hoveredEntry_ = i;
            emit paperClicked(entry.paperId);
            update();
            return;
        }
    }
}
