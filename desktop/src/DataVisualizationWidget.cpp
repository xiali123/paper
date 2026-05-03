#include "DataVisualizationWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QInputDialog>
#include <QSplitter>
#include <cmath>

DataVisualizationWidget::DataVisualizationWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void DataVisualizationWidget::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: data table
    auto* leftPanel = new QVBoxLayout();

    auto* toolbar = new QHBoxLayout();
    chartTypeCombo_ = new QComboBox();
    chartTypeCombo_->addItems({"Bar Chart", "Pie Chart", "Line Chart"});
    connect(chartTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DataVisualizationWidget::onChartTypeChanged);
    toolbar->addWidget(new QLabel("Type:"));
    toolbar->addWidget(chartTypeCombo_, 1);

    refreshBtn_ = new QPushButton("Refresh");
    connect(refreshBtn_, &QPushButton::clicked, this, &DataVisualizationWidget::onRefresh);
    toolbar->addWidget(refreshBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &DataVisualizationWidget::onExport);
    toolbar->addWidget(exportBtn_);

    leftPanel->addLayout(toolbar);

    dataTable_ = new QTableWidget();
    dataTable_->setColumnCount(3);
    dataTable_->setHorizontalHeaderLabels({"Label", "Value", "Color"});
    dataTable_->horizontalHeader()->setStretchLastSection(true);
    dataTable_->setColumnWidth(0, 120);
    dataTable_->setColumnWidth(1, 80);
    leftPanel->addWidget(dataTable_, 1);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &DataVisualizationWidget::onAddDataPoint);
    btnRow->addWidget(addBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &DataVisualizationWidget::onRemoveDataPoint);
    btnRow->addWidget(removeBtn_);

    leftPanel->addLayout(btnRow);

    auto* leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    splitter->addWidget(leftWidget);

    // Right: chart canvas
    auto* rightPanel = new QVBoxLayout();
    titleLabel_ = new QLabel("Chart Preview");
    titleLabel_->setStyleSheet("font-weight: bold; font-size: 14px;");
    titleLabel_->setAlignment(Qt::AlignCenter);
    rightPanel->addWidget(titleLabel_);

    auto* canvas = new QWidget();
    canvas->setMinimumSize(350, 300);
    rightPanel->addWidget(canvas, 1);

    auto* rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter);

    // Default sample data
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};
    QStringList labels = {"Machine Learning", "NLP", "Computer Vision", "Robotics", "Security"};
    double values[] = {45, 32, 28, 15, 22};
    for (int i = 0; i < 5; ++i) {
        ChartDataPoint pt;
        pt.label = labels[i];
        pt.value = values[i];
        pt.color = colors[i];
        dataPoints_.append(pt);
    }
    refreshDataTable();
}

void DataVisualizationWidget::setBarData(const QMap<QString, double>& data) {
    dataPoints_.clear();
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246),
                       QColor(236,72,153), QColor(14,165,233), QColor(168,85,247)};
    int i = 0;
    for (auto it = data.begin(); it != data.end(); ++it) {
        ChartDataPoint pt;
        pt.label = it.key();
        pt.value = it.value();
        pt.color = colors[i % 8];
        dataPoints_.append(pt);
        i++;
    }
    chartType_ = 0;
    chartTypeCombo_->setCurrentIndex(0);
    refreshDataTable();
    update();
}

void DataVisualizationWidget::setPieData(const QMap<QString, double>& data) {
    setBarData(data);
    chartType_ = 1;
    chartTypeCombo_->setCurrentIndex(1);
}

void DataVisualizationWidget::setLineData(const QList<QPair<QString, double>>& points) {
    dataPoints_.clear();
    QColor lineColor(59, 130, 246);
    for (const auto& [label, value] : points) {
        ChartDataPoint pt;
        pt.label = label;
        pt.value = value;
        pt.color = lineColor;
        dataPoints_.append(pt);
    }
    chartType_ = 2;
    chartTypeCombo_->setCurrentIndex(2);
    refreshDataTable();
    update();
}

void DataVisualizationWidget::setMultiSeriesData(const QList<ChartSeries>& series) {
    multiSeries_ = series;
    update();
}

void DataVisualizationWidget::setTitle(const QString& title) {
    chartTitle_ = title;
    titleLabel_->setText(title);
}

void DataVisualizationWidget::exportChart(const QString& filePath) {
    QPixmap pixmap(size());
    render(&pixmap);
    pixmap.save(filePath);
    emit chartExported(filePath);
}

void DataVisualizationWidget::onChartTypeChanged(int index) {
    chartType_ = index;
    QStringList types = {"bar", "pie", "line"};
    emit chartTypeChanged(types.value(index, "bar"));
    update();
}

void DataVisualizationWidget::onRefresh() {
    syncDataFromTable();
    update();
}

void DataVisualizationWidget::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Chart", "chart.png",
        "PNG (*.png);;JPEG (*.jpg);;SVG (*.svg)");
    if (!path.isEmpty()) exportChart(path);
}

void DataVisualizationWidget::onAddDataPoint() {
    QString label = QInputDialog::getText(this, "Add Data Point", "Label:");
    if (label.isEmpty()) return;
    bool ok = false;
    double value = QInputDialog::getDouble(this, "Value", "Value:", 10, 0, 1e9, 2, &ok);
    if (!ok) return;

    ChartDataPoint pt;
    pt.label = label;
    pt.value = value;
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};
    pt.color = colors[dataPoints_.size() % 5];
    dataPoints_.append(pt);
    refreshDataTable();
    update();
}

void DataVisualizationWidget::onRemoveDataPoint() {
    int row = dataTable_->currentRow();
    if (row < 0 || row >= dataPoints_.size()) return;
    dataPoints_.removeAt(row);
    refreshDataTable();
    update();
}

void DataVisualizationWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Find chart canvas area (right panel, below title)
    QWidget* rightParent = titleLabel_->parentWidget();
    if (!rightParent) return;
    QRect canvasRect = rightParent->geometry();
    // Offset to this widget's coordinates
    QPoint offset = rightParent->mapTo(this, QPoint(0, 0));
    canvasRect = QRect(offset.x() + 20, offset.y() + titleLabel_->height() + 10,
                        canvasRect.width() - 40, canvasRect.height() - titleLabel_->height() - 30);

    if (canvasRect.width() < 50 || canvasRect.height() < 50) return;

    p.fillRect(canvasRect.adjusted(-5, -5, 5, 5), QColor(255, 255, 255));

    switch (chartType_) {
        case 0: drawBarChart(p, canvasRect); break;
        case 1: drawPieChart(p, canvasRect); break;
        case 2: drawLineChart(p, canvasRect); break;
    }
}

void DataVisualizationWidget::drawBarChart(QPainter& p, const QRect& rect) {
    if (dataPoints_.isEmpty()) return;

    double maxVal = 0;
    for (const auto& pt : dataPoints_) maxVal = qMax(maxVal, pt.value);
    if (maxVal <= 0) maxVal = 1;

    drawAxes(p, rect, maxVal);

    int n = dataPoints_.size();
    qreal chartLeft = rect.left() + 50;
    qreal chartBottom = rect.bottom() - 30;
    qreal chartWidth = rect.width() - 70;
    qreal chartHeight = rect.height() - 50;
    qreal barWidth = chartWidth / (n * 1.5);

    for (int i = 0; i < n; ++i) {
        qreal x = chartLeft + i * (chartWidth / n) + (chartWidth / n - barWidth) / 2;
        qreal h = (dataPoints_[i].value / maxVal) * chartHeight;
        qreal y = chartBottom - h;

        // Gradient fill
        QLinearGradient grad(x, y, x, chartBottom);
        grad.setColorAt(0, dataPoints_[i].color);
        grad.setColorAt(1, dataPoints_[i].color.lighter(150));
        p.setBrush(grad);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(x, y, barWidth, h), 3, 3);

        // Value label
        p.setPen(QColor(51, 65, 85));
        QFont font = p.font();
        font.setPixelSize(10);
        p.setFont(font);
        p.drawText(QRectF(x - 10, y - 18, barWidth + 20, 16), Qt::AlignCenter,
                   QString::number(dataPoints_[i].value, 'f', 0));

        // X label
        p.drawText(QRectF(x - 20, chartBottom + 5, barWidth + 40, 20), Qt::AlignCenter,
                   dataPoints_[i].label);
    }
}

void DataVisualizationWidget::drawPieChart(QPainter& p, const QRect& rect) {
    if (dataPoints_.isEmpty()) return;

    double total = 0;
    for (const auto& pt : dataPoints_) total += pt.value;
    if (total <= 0) return;

    int cx = rect.center().x();
    int cy = rect.center().y();
    int r = qMin(rect.width(), rect.height()) / 2 - 30;

    qreal startAngle = 0;
    for (int i = 0; i < dataPoints_.size(); ++i) {
        qreal span = (dataPoints_[i].value / total) * 360 * 16;

        p.setBrush(dataPoints_[i].color);
        p.setPen(QColor(255, 255, 255));
        p.drawPie(cx - r, cy - r, r * 2, r * 2,
                  static_cast<int>(startAngle), static_cast<int>(span));

        // Label
        qreal midAngle = startAngle / 16.0 + (span / 16.0) / 2.0;
        qreal labelR = r * 0.65;
        qreal lx = cx + labelR * cos(midAngle * M_PI / 180.0);
        qreal ly = cy - labelR * sin(midAngle * M_PI / 180.0);
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPixelSize(10);
        font.setBold(true);
        p.setFont(font);
        double pct = dataPoints_[i].value / total * 100;
        p.drawText(QRectF(lx - 30, ly - 8, 60, 16), Qt::AlignCenter,
                   QString("%1\n%2%").arg(dataPoints_[i].label.left(8)).arg(pct, 0, 'f', 0));

        startAngle += span;
    }

    drawLegend(p, rect);
}

void DataVisualizationWidget::drawLineChart(QPainter& p, const QRect& rect) {
    if (dataPoints_.size() < 2) return;

    double maxVal = 0;
    for (const auto& pt : dataPoints_) maxVal = qMax(maxVal, pt.value);
    if (maxVal <= 0) maxVal = 1;

    drawAxes(p, rect, maxVal);

    qreal chartLeft = rect.left() + 50;
    qreal chartBottom = rect.bottom() - 30;
    qreal chartWidth = rect.width() - 70;
    qreal chartHeight = rect.height() - 50;
    int n = dataPoints_.size();

    QPolygonF polygon;
    for (int i = 0; i < n; ++i) {
        qreal x = chartLeft + i * chartWidth / (n - 1);
        qreal y = chartBottom - (dataPoints_[i].value / maxVal) * chartHeight;
        polygon.append(QPointF(x, y));

        // X label
        p.setPen(QColor(51, 65, 85));
        QFont font = p.font();
        font.setPixelSize(9);
        p.setFont(font);
        p.drawText(QRectF(x - 25, chartBottom + 5, 50, 20), Qt::AlignCenter,
                   dataPoints_[i].label);
    }

    // Area fill
    QLinearGradient grad(0, rect.top(), 0, chartBottom);
    grad.setColorAt(0, QColor(59, 130, 246, 60));
    grad.setColorAt(1, QColor(59, 130, 246, 10));
    QPolygonF area = polygon;
    area.append(QPointF(polygon.last().x(), chartBottom));
    area.append(QPointF(polygon.first().x(), chartBottom));
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawPolygon(area);

    // Line
    QPen linePen(QColor(59, 130, 246), 2);
    p.setPen(linePen);
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(polygon);

    // Dots
    for (const auto& pt : polygon) {
        p.setBrush(QColor(59, 130, 246));
        p.setPen(QColor(255, 255, 255));
        p.drawEllipse(pt, 4, 4);
    }
}

void DataVisualizationWidget::drawLegend(QPainter& p, const QRect& rect) {
    int lx = rect.right() - 130;
    int ly = rect.top() + 10;

    QFont font = p.font();
    font.setPixelSize(10);
    p.setFont(font);

    for (int i = 0; i < dataPoints_.size(); ++i) {
        p.setBrush(dataPoints_[i].color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(lx, ly + i * 18, 12, 12, 2, 2);

        p.setPen(QColor(51, 65, 85));
        p.drawText(lx + 18, ly + i * 18 + 11, dataPoints_[i].label);
    }
}

void DataVisualizationWidget::drawAxes(QPainter& p, const QRect& rect, double maxVal) {
    qreal chartLeft = rect.left() + 50;
    qreal chartBottom = rect.bottom() - 30;
    qreal chartTop = rect.top() + 10;

    QPen axisPen(QColor(200, 200, 200), 1);
    p.setPen(axisPen);
    p.drawLine(QPointF(chartLeft, chartTop), QPointF(chartLeft, chartBottom));
    p.drawLine(QPointF(chartLeft, chartBottom), QPointF(rect.right() - 20, chartBottom));

    QFont font = p.font();
    font.setPixelSize(9);
    p.setFont(font);
    p.setPen(QColor(148, 163, 184));

    int steps = 5;
    for (int i = 0; i <= steps; ++i) {
        qreal y = chartBottom - i * (chartBottom - chartTop) / steps;
        double val = i * maxVal / steps;
        p.drawText(QRectF(chartLeft - 48, y - 8, 44, 16), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(val, 'f', 0));
        if (i > 0) {
            p.setPen(QPen(QColor(240, 240, 240), 1, Qt::DashLine));
            p.drawLine(QPointF(chartLeft, y), QPointF(rect.right() - 20, y));
            p.setPen(QColor(148, 163, 184));
        }
    }
}

void DataVisualizationWidget::refreshDataTable() {
    dataTable_->setRowCount(dataPoints_.size());
    for (int i = 0; i < dataPoints_.size(); ++i) {
        dataTable_->setItem(i, 0, new QTableWidgetItem(dataPoints_[i].label));
        dataTable_->setItem(i, 1, new QTableWidgetItem(QString::number(dataPoints_[i].value, 'f', 1)));
        auto* colorItem = new QTableWidgetItem(dataPoints_[i].color.name());
        colorItem->setForeground(dataPoints_[i].color);
        dataTable_->setItem(i, 2, colorItem);
    }
}

void DataVisualizationWidget::syncDataFromTable() {
    for (int i = 0; i < dataTable_->rowCount() && i < dataPoints_.size(); ++i) {
        auto* labelItem = dataTable_->item(i, 0);
        auto* valueItem = dataTable_->item(i, 1);
        if (labelItem) dataPoints_[i].label = labelItem->text();
        if (valueItem) dataPoints_[i].value = valueItem->text().toDouble();
    }
}
