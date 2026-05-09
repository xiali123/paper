#include "visualization/PaperTrendHeatmap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperTrendHeatmap::PaperTrendHeatmap(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperTrendHeatmap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Color:"));
    colorCombo_ = new QComboBox();
    colorCombo_->addItems({"Blue-Green", "Red-Yellow", "Purple-Orange", "Grayscale"});
    connect(colorCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperTrendHeatmap::onColorScheme);
    toolbar->addWidget(colorCombo_);

    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperTrendHeatmap::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTrendHeatmap::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Generate trend heatmap");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 480);
}

void PaperTrendHeatmap::setData(const QMap<QString, QMap<QString, qreal>>& data) {
    rawData_ = data;
    rebuildCells();
    updateInfo();
    update();
}

QList<TrendCell> PaperTrendHeatmap::cells() const { return cells_; }
QStringList PaperTrendHeatmap::topics() const { return topics_; }
QStringList PaperTrendHeatmap::years() const { return years_; }

void PaperTrendHeatmap::onGenerate() {
    rawData_.clear();
    QStringList topicList = {"Deep Learning", "NLP", "Computer Vision", "Reinforcement Learning",
                            "Generative AI", "Graph NN", "Multimodal", "Optimization",
                            "Federated Learning", "Explainability"};
    QStringList yearList;
    for (int y = 2018; y <= 2026; ++y) yearList.append(QString::number(y));

    for (const auto& topic : topicList) {
        for (int i = 0; i < yearList.size(); ++i) {
            qreal trend = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
            if (i > 4 && topic == "Generative AI") trend = qMax(trend, 0.7);
            if (i > 6 && topic == "Multimodal") trend = qMax(trend, 0.6);
            rawData_[topic][yearList[i]] = trend;
        }
    }
    rebuildCells();
    updateInfo();
    update();
}

void PaperTrendHeatmap::onColorScheme(int) { rebuildCells(); update(); }
void PaperTrendHeatmap::onClear() {
    rawData_.clear();
    cells_.clear();
    topics_.clear();
    years_.clear();
    infoLabel_->setText("Generate trend heatmap");
    update();
}

void PaperTrendHeatmap::rebuildCells() {
    cells_.clear();
    topics_ = rawData_.keys();
    std::sort(topics_.begin(), topics_.end());

    years_.clear();
    if (!topics_.isEmpty()) {
        years_ = rawData_[topics_.first()].keys();
        std::sort(years_.begin(), years_.end());
    }

    qreal maxVal = 0;
    for (const auto& topic : topics_) {
        for (const auto& year : years_) {
            qreal v = rawData_[topic][year];
            if (v > maxVal) maxVal = v;
        }
    }

    int scheme = colorCombo_->currentIndex();
    for (const auto& topic : topics_) {
        for (const auto& year : years_) {
            TrendCell c;
            c.topic = topic;
            c.year = year;
            c.value = rawData_[topic][year];
            qreal t = maxVal > 0 ? c.value / maxVal : 0;

            if (scheme == 0) c.color = QColor::fromHsvF(0.55 - t * 0.3, 0.6 + t * 0.3, 0.4 + t * 0.5);
            else if (scheme == 1) c.color = QColor::fromHsvF(0.0 + t * 0.15, 0.7 + t * 0.2, 0.4 + t * 0.5);
            else if (scheme == 2) c.color = QColor::fromHsvF(0.75 - t * 0.2, 0.5 + t * 0.4, 0.4 + t * 0.5);
            else c.color = QColor::fromHsvF(0, 0, 0.3 + t * 0.65);

            cells_.append(c);
        }
    }
}

void PaperTrendHeatmap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (cells_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate trend heatmap");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Trend Heatmap");

    int w = width(), h = height();
    drawHeatmap(p, QRect(20, 50, w - 40, h / 2 + 30));
    drawLegend(p, QRect(20, h / 2 + 90, w / 2 - 30, 30));
    drawTopTrends(p, QRect(w / 2 + 10, h / 2 + 90, w / 2 - 30, h / 2 - 100));
}

void PaperTrendHeatmap::drawHeatmap(QPainter& p, const QRect& rect) {
    int cols = years_.size();
    int rows = topics_.size();
    if (cols == 0 || rows == 0) return;

    int labelW = 110;
    int labelH = 22;
    int cellW = (rect.width() - labelW) / cols;
    int cellH = qMin(24, (rect.height() - labelH) / rows);

    // Year headers
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    for (int c = 0; c < cols; ++c) {
        int x = rect.x() + labelW + c * cellW;
        p.drawText(x, rect.y(), cellW, labelH, Qt::AlignCenter, years_[c]);
    }

    for (int r = 0; r < rows; ++r) {
        int y = rect.y() + labelH + r * cellH;

        // Topic label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y, labelW - 5, cellH, Qt::AlignRight | Qt::AlignVCenter,
                   topics_[r].left(16));

        for (int c = 0; c < cols; ++c) {
            int x = rect.x() + labelW + c * cellW;
            const auto& cell = cells_[r * cols + c];

            p.setPen(Qt::NoPen);
            p.setBrush(cell.color);
            p.drawRoundedRect(x + 1, y + 1, cellW - 2, cellH - 2, 2, 2);

            if (cellW > 30 && cellH > 16) {
                p.setPen(cell.value > 0.5 ? Qt::white : QColor(15, 23, 42));
                p.setFont(QFont("Arial", 6));
                p.drawText(x + 1, y + 1, cellW - 2, cellH - 2, Qt::AlignCenter,
                           QString::number(cell.value, 'f', 1));
            }
        }
    }
}

void PaperTrendHeatmap::drawLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x(), rect.y() + 12, "Low");

    int barW = rect.width() - 60;
    for (int i = 0; i < barW; ++i) {
        qreal t = static_cast<qreal>(i) / barW;
        QColor c;
        int scheme = colorCombo_->currentIndex();
        if (scheme == 0) c = QColor::fromHsvF(0.55 - t * 0.3, 0.6 + t * 0.3, 0.4 + t * 0.5);
        else if (scheme == 1) c = QColor::fromHsvF(0.0 + t * 0.15, 0.7 + t * 0.2, 0.4 + t * 0.5);
        else if (scheme == 2) c = QColor::fromHsvF(0.75 - t * 0.2, 0.5 + t * 0.4, 0.4 + t * 0.5);
        else c = QColor::fromHsvF(0, 0, 0.3 + t * 0.65);
        p.setPen(c);
        p.drawLine(rect.x() + 25 + i, rect.y() + 2, rect.x() + 25 + i, rect.y() + 16);
    }

    p.setPen(QColor(100, 116, 139));
    p.drawText(rect.x() + barW + 30, rect.y() + 12, "High");
}

void PaperTrendHeatmap::drawTopTrends(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Fastest Growing");

    if (years_.size() < 2) return;

    QMap<QString, qreal> growth;
    for (const auto& topic : topics_) {
        qreal recent = rawData_[topic][years_.last()];
        qreal early = rawData_[topic][years_.first()];
        growth[topic] = recent - early;
    }

    QList<QPair<QString, qreal>> sorted;
    for (auto it = growth.begin(); it != growth.end(); ++it) sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    int show = qMin(6, sorted.size());
    int barH = qMin(18, (rect.height() - 30) / show);
    qreal maxGrowth = qMax(0.01, qAbs(sorted.first().second));

    for (int i = 0; i < show; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int barW = static_cast<int>((qAbs(sorted[i].second) / maxGrowth) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, sorted[i].first.left(12));

        p.setPen(Qt::NoPen);
        p.setBrush(sorted[i].second >= 0 ? QColor(16, 185, 129) : QColor(239, 68, 68));
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2,
                   QString::number(sorted[i].second, 'f', 2));
    }
}

void PaperTrendHeatmap::updateInfo() {
    if (cells_.isEmpty()) { infoLabel_->setText("Generate trend heatmap"); return; }
    infoLabel_->setText(QString("%1 topics | %2 years | %3 cells")
        .arg(topics_.size()).arg(years_.size()).arg(cells_.size()));
}
