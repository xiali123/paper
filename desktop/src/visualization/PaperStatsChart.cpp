#include "visualization/PaperStatsChart.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

PaperStatsChart::PaperStatsChart(QWidget* parent)
    : QWidget(parent)
{
    colors_ = {"#3b82f6", "#10b981", "#f59e0b", "#ef4444", "#8b5cf6",
               "#ec4899", "#06b6d4", "#84cc16", "#f97316", "#6366f1"};
    setMinimumSize(300, 250);
}

void PaperStatsChart::setChartData(const QMap<QString, double>& data, ChartType type) {
    data_ = data;
    chartType_ = type;
    update();
}

void PaperStatsChart::setTitle(const QString& title) {
    title_ = title;
    update();
}

void PaperStatsChart::setColorScheme(const QStringList& colors) {
    colors_ = colors;
    update();
}

void PaperStatsChart::setLegendVisible(bool visible) {
    legendVisible_ = visible;
    update();
}

void PaperStatsChart::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().base());

    QRect chartRect = rect().adjusted(40, 30, -20, -40);

    if (!title_.isEmpty()) {
        paintTitle(painter, chartRect);
        chartRect.setTop(chartRect.top() + 25);
    }

    if (legendVisible_ && !data_.isEmpty()) {
        chartRect.setRight(chartRect.right() - 120);
    }

    if (data_.isEmpty()) {
        painter.setPen(palette().mid().color());
        painter.drawText(chartRect, Qt::AlignCenter, "No data");
        return;
    }

    switch (chartType_) {
        case BarChart: paintBarChart(painter, chartRect); break;
        case PieChart: paintPieChart(painter, chartRect); break;
        case LineChart: paintLineChart(painter, chartRect); break;
    }

    if (legendVisible_) {
        QRect legendRect(rect().right() - 130, chartRect.top(), 120, chartRect.height());
        paintLegend(painter, legendRect);
    }
}

void PaperStatsChart::paintBarChart(QPainter& painter, const QRect& rect) {
    QStringList labels = data_.keys();
    int n = labels.size();
    if (n == 0) return;

    double maxVal = 0;
    for (const auto& v : data_) maxVal = qMax(maxVal, v);
    if (maxVal == 0) return;

    int barWidth = qMax(10, (rect.width() - 40) / n - 8);
    int gap = (rect.width() - n * barWidth) / (n + 1);

    // Y-axis
    painter.setPen(QPen(palette().mid().color(), 1));
    painter.drawLine(rect.left(), rect.top(), rect.left(), rect.bottom());
    painter.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

    // Grid lines
    painter.setPen(QPen(QColor(0, 0, 0, 30), 1, Qt::DotLine));
    for (int i = 1; i <= 4; ++i) {
        int y = rect.bottom() - (rect.height() * i / 4);
        painter.drawLine(rect.left(), y, rect.right(), y);
        painter.setPen(palette().mid().color());
        painter.setFont(QFont("Consolas", 8));
        painter.drawText(rect.left() - 35, y - 8, 30, 16, Qt::AlignRight | Qt::AlignVCenter,
                         QString::number(maxVal * i / 4, 'f', 0));
        painter.setPen(QPen(QColor(0, 0, 0, 30), 1, Qt::DotLine));
    }

    // Bars
    for (int i = 0; i < n; ++i) {
        double val = data_[labels[i]];
        int barHeight = static_cast<int>(rect.height() * val / maxVal);
        int x = rect.left() + gap + i * (barWidth + gap);
        int y = rect.bottom() - barHeight;

        QColor color(colors_[i % colors_.size()]);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);

        QPainterPath path;
        path.addRoundedRect(x, y, barWidth, barHeight, 4, 4);
        painter.drawPath(path);

        // Value on top
        painter.setPen(palette().text().color());
        painter.setFont(QFont("Consolas", 8));
        painter.drawText(x - 5, y - 14, barWidth + 10, 14, Qt::AlignCenter,
                         QString::number(val, 'f', 0));

        // Label below
        painter.drawText(x - 10, rect.bottom() + 4, barWidth + 20, 16,
                         Qt::AlignCenter, labels[i].left(6));
    }
}

void PaperStatsChart::paintPieChart(QPainter& painter, const QRect& rect) {
    double total = 0;
    for (const auto& v : data_) total += v;
    if (total == 0) return;

    int size = qMin(rect.width(), rect.height()) - 20;
    QRect pieRect(rect.center().x() - size / 2, rect.center().y() - size / 2, size, size);

    double startAngle = 0;
    QStringList labels = data_.keys();

    for (int i = 0; i < labels.size(); ++i) {
        double val = data_[labels[i]];
        double span = 360.0 * val / total;

        painter.setBrush(QColor(colors_[i % colors_.size()]));
        painter.setPen(QPen(palette().base().color(), 2));
        painter.drawPie(pieRect, static_cast<int>(startAngle * 16), static_cast<int>(span * 16));

        startAngle += span;
    }
}

void PaperStatsChart::paintLineChart(QPainter& painter, const QRect& rect) {
    QStringList labels = data_.keys();
    int n = labels.size();
    if (n == 0) return;

    double maxVal = 0;
    for (const auto& v : data_) maxVal = qMax(maxVal, v);
    if (maxVal == 0) return;

    // Grid
    painter.setPen(QPen(QColor(0, 0, 0, 30), 1, Qt::DotLine));
    for (int i = 1; i <= 4; ++i) {
        int y = rect.bottom() - (rect.height() * i / 4);
        painter.drawLine(rect.left(), y, rect.right(), y);
    }

    // Axes
    painter.setPen(QPen(palette().mid().color(), 1));
    painter.drawLine(rect.left(), rect.top(), rect.left(), rect.bottom());
    painter.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());

    // Points + line
    QPainterPath path;
    QVector<QPoint> points;
    int step = (n > 1) ? rect.width() / (n - 1) : rect.width() / 2;

    for (int i = 0; i < n; ++i) {
        double val = data_[labels[i]];
        int x = rect.left() + i * step;
        int y = rect.bottom() - static_cast<int>(rect.height() * val / maxVal);
        points << QPoint(x, y);

        if (i == 0) path.moveTo(x, y);
        else path.lineTo(x, y);
    }

    // Fill area
    QPainterPath fill = path;
    fill.lineTo(points.last().x(), rect.bottom());
    fill.lineTo(points.first().x(), rect.bottom());
    fill.closeSubpath();
    QColor areaColor(colors_[0]);
    areaColor.setAlpha(40);
    painter.fillPath(fill, areaColor);

    // Line
    painter.setPen(QPen(QColor(colors_[0]), 2));
    painter.drawPath(path);

    // Dots
    painter.setBrush(QColor(colors_[0]));
    for (const auto& pt : points) {
        painter.drawEllipse(pt, 4, 4);
    }

    // Labels
    painter.setPen(palette().text().color());
    painter.setFont(QFont("Consolas", 8));
    for (int i = 0; i < n; ++i) {
        int x = points[i].x();
        painter.drawText(x - 15, rect.bottom() + 4, 30, 16, Qt::AlignCenter, labels[i].left(6));
    }
}

void PaperStatsChart::paintLegend(QPainter& painter, const QRect& rect) {
    QStringList labels = data_.keys();
    painter.setFont(QFont("Consolas", 9));

    int y = rect.top();
    for (int i = 0; i < labels.size(); ++i) {
        painter.setBrush(QColor(colors_[i % colors_.size()]));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect.left(), y, 12, 12, 2, 2);

        painter.setPen(palette().text().color());
        painter.drawText(rect.left() + 16, y, rect.width() - 16, 16, Qt::AlignVCenter,
                         labels[i]);
        y += 20;
    }
}

void PaperStatsChart::paintTitle(QPainter& painter, const QRect&) {
    painter.setPen(palette().text().color());
    painter.setFont(QFont("", 12, QFont::Bold));
    painter.drawText(rect().adjusted(40, 5, 0, 0), Qt::AlignLeft | Qt::AlignTop, title_);
}
