#include "analysis/ResearchTrendAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

ResearchTrendAnalyzer::ResearchTrendAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ResearchTrends")
{
    setupUI();
    loadSettings();
}

void ResearchTrendAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Chart:"));
    chartCombo_ = new QComboBox();
    chartCombo_->addItems({"Trend Lines", "Bar Comparison", "Bubble", "Top List"});
    connect(chartCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ResearchTrendAnalyzer::onChartTypeChanged);
    toolbar->addWidget(chartCombo_, 1);

    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"5 Years", "10 Years", "All"});
    connect(periodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ResearchTrendAnalyzer::onPeriodChanged);
    toolbar->addWidget(periodCombo_, 1);

    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &ResearchTrendAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    resetBtn_ = new QPushButton("Reset");
    resetBtn_->setStyleSheet("color: #dc2626;");
    connect(resetBtn_, &QPushButton::clicked, this, &ResearchTrendAnalyzer::onReset);
    toolbar->addWidget(resetBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Click Analyze to detect trends");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 400);
}

void ResearchTrendAnalyzer::setPapers(const QList<QPair<int, QString>>& papers) {
    Q_UNUSED(papers);
}

void ResearchTrendAnalyzer::addYearData(int year, const QMap<QString, int>& topics) {
    TrendYear ty;
    ty.year = year;
    ty.topicCounts = topics;
    years_.append(ty);
}

QList<TrendTopic> ResearchTrendAnalyzer::topTrends(int limit) const {
    QList<TrendTopic> sorted = topics_;
    std::sort(sorted.begin(), sorted.end(),
        [](const TrendTopic& a, const TrendTopic& b) { return a.count > b.count; });
    if (sorted.size() > limit) sorted = sorted.mid(0, limit);
    return sorted;
}

QList<TrendYear> ResearchTrendAnalyzer::yearlyData() const { return years_; }

void ResearchTrendAnalyzer::onChartTypeChanged(int) { update(); }
void ResearchTrendAnalyzer::onPeriodChanged(int) { update(); }

void ResearchTrendAnalyzer::onAnalyze() {
    topics_.clear();
    years_.clear();

    QStringList topicNames = {"Transformer", "GAN", "Diffusion Model", "RLHF", "Multimodal",
        "LLM", "Computer Vision", "NLP", "Graph NN", "Federated Learning",
        "Self-Supervised", "Contrastive Learning", "Neural Architecture", "Knowledge Distillation"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
        QColor(239,68,68), QColor(139,92,246), QColor(236,72,153), QColor(14,165,233),
        QColor(168,85,247), QColor(234,179,8), QColor(34,197,94)};

    int startYear = (periodCombo_->currentIndex() == 0) ? 2022 : (periodCombo_->currentIndex() == 1) ? 2017 : 2015;
    for (int y = startYear; y <= 2026; ++y) {
        TrendYear ty;
        ty.year = y;
        for (int t = 0; t < 10; ++t) {
            int base = 5 + QRandomGenerator::global()->bounded(40);
            int growth = (y - startYear) * QRandomGenerator::global()->bounded(5);
            ty.topicCounts[topicNames[t]] = base + growth;
        }
        years_.append(ty);
    }

    for (int t = 0; t < 10; ++t) {
        TrendTopic topic;
        topic.name = topicNames[t];
        topic.color = colors[t];
        int firstCount = years_.first().topicCounts.value(topic.name, 0);
        int lastCount = years_.last().topicCounts.value(topic.name, 0);
        topic.count = lastCount;
        topic.growth = firstCount > 0 ? static_cast<qreal>(lastCount - firstCount) / firstCount : 0;
        topics_.append(topic);
    }

    saveSettings();
    emit analysisComplete(topics_.size());
    updateInfo();
    update();
}

void ResearchTrendAnalyzer::onReset() {
    topics_.clear();
    years_.clear();
    saveSettings();
    update();
}

void ResearchTrendAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (topics_.isEmpty() || years_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Click Analyze to detect research trends");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 50, "Research Trend Analysis");

    QRect chartRect(40, 70, width() - 80, height() - 100);
    QString chartType = chartCombo_->currentText();
    if (chartType == "Trend Lines") drawTrendLines(p, chartRect);
    else if (chartType == "Bar Comparison") drawBarComparison(p, chartRect);
    else if (chartType == "Bubble") drawBubbleChart(p, chartRect);
    else drawTopList(p, chartRect);
}

void ResearchTrendAnalyzer::drawTrendLines(QPainter& p, const QRect& rect) {
    // Axes
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(rect.bottomLeft(), rect.bottomRight());
    p.drawLine(rect.bottomLeft(), rect.topLeft());

    int maxY = 1;
    for (const auto& y : years_) {
        for (const auto& v : y.topicCounts) maxY = qMax(maxY, v);
    }

    // Y axis labels
    p.setFont(QFont("Arial", 8));
    p.setPen(QColor(148, 163, 184));
    for (int i = 0; i <= 4; ++i) {
        int val = maxY * i / 4;
        int y = rect.bottom() - (rect.height() * i / 4);
        p.drawText(rect.x() - 30, y + 4, QString::number(val));
        p.setPen(QPen(QColor(241, 245, 249), 1));
        p.drawLine(rect.x(), y, rect.right(), y);
        p.setPen(QColor(148, 163, 184));
    }

    // X axis labels
    for (int i = 0; i < years_.size(); ++i) {
        qreal x = rect.x() + (static_cast<qreal>(i) / qMax(1, years_.size() - 1)) * rect.width();
        p.drawText(static_cast<int>(x) - 15, rect.bottom() + 15, QString::number(years_[i].year));
    }

    // Lines for top 5 topics
    auto top5 = topTrends(5);
    for (const auto& topic : top5) {
        QPolygonF points;
        for (int i = 0; i < years_.size(); ++i) {
            qreal x = rect.x() + (static_cast<qreal>(i) / qMax(1, years_.size() - 1)) * rect.width();
            int val = years_[i].topicCounts.value(topic.name, 0);
            qreal y = rect.bottom() - (static_cast<qreal>(val) / maxY) * rect.height();
            points << QPointF(x, y);
        }
        p.setPen(QPen(topic.color, 2));
        p.setBrush(Qt::NoBrush);
        p.drawPolyline(points);

        // Label at end
        if (!points.isEmpty()) {
            p.setFont(QFont("Arial", 8));
            p.drawText(points.last() + QPointF(5, 4), topic.name);
        }
    }
}

void ResearchTrendAnalyzer::drawBarComparison(QPainter& p, const QRect& rect) {
    auto top6 = topTrends(6);
    if (top6.isEmpty()) return;

    int barW = (rect.width() - 40) / top6.size();
    int maxVal = 1;
    for (const auto& t : top6) maxVal = qMax(maxVal, t.count);

    for (int i = 0; i < top6.size(); ++i) {
        int x = rect.x() + 20 + i * barW;
        qreal h = (static_cast<qreal>(top6[i].count) / maxVal) * (rect.height() - 40);

        p.setPen(Qt::NoPen);
        p.setBrush(top6[i].color.lighter(180));
        p.drawRoundedRect(x, rect.bottom() - 30 - static_cast<int>(h), barW - 8, static_cast<int>(h), 4, 4);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(x, rect.bottom() - 10, barW - 8, 20, Qt::AlignCenter, top6[i].name.left(8));
        p.setFont(QFont("Arial", 8));
        p.drawText(x, rect.bottom() - 35 - static_cast<int>(h), barW - 8, 15, Qt::AlignCenter, QString::number(top6[i].count));
    }
}

void ResearchTrendAnalyzer::drawBubbleChart(QPainter& p, const QRect& rect) {
    for (int i = 0; i < topics_.size(); ++i) {
        const auto& t = topics_[i];
        qreal x = rect.x() + 30 + (i % 5) * (rect.width() - 60) / 5;
        qreal y = rect.y() + 20 + (i / 5) * (rect.height() - 40) / 2;
        qreal r = 10 + qMin(t.count * 0.5, 40.0);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(t.color.red(), t.color.green(), t.color.blue(), 120));
        p.drawEllipse(QPointF(x, y), r, r);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(QRectF(x - r, y - r, r * 2, r * 2), Qt::AlignCenter, t.name.left(10));
    }
}

void ResearchTrendAnalyzer::drawTopList(QPainter& p, const QRect& rect) {
    auto sorted = topTrends(10);
    int barH = qMin(24, (rect.height() - 20) / sorted.size());
    int maxVal = 1;
    for (const auto& t : sorted) maxVal = qMax(maxVal, t.count);

    for (int i = 0; i < sorted.size(); ++i) {
        int y = rect.y() + i * (barH + 4);
        qreal w = (static_cast<qreal>(sorted[i].count) / maxVal) * (rect.width() - 140);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 6, 120, 20, Qt::AlignRight | Qt::AlignVCenter, sorted[i].name);

        p.setPen(Qt::NoPen);
        p.setBrush(sorted[i].color);
        p.drawRoundedRect(rect.x() + 125, y, static_cast<int>(w), barH - 2, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.drawText(rect.x() + 128 + static_cast<int>(w), y + barH - 6,
                   QString("+%1%").arg(sorted[i].growth * 100, 0, 'f', 0));
    }
}

void ResearchTrendAnalyzer::updateInfo() {
    if (topics_.isEmpty()) { infoLabel_->setText("Click Analyze to detect trends"); return; }
    auto top3 = topTrends(3);
    QStringList names;
    for (const auto& t : top3) names << t.name;
    infoLabel_->setText(QString("Top trends: %1 | %2 topics across %3 years")
        .arg(names.join(", ")).arg(topics_.size()).arg(years_.size()));
}

void ResearchTrendAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("years");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TrendYear ty;
        ty.year = settings_.value("year").toInt();
        QMap<QString, QVariant> map = settings_.value("topics").toMap();
        for (auto it = map.begin(); it != map.end(); ++it) ty.topicCounts[it.key()] = it.value().toInt();
        years_.append(ty);
    }
    settings_.endArray();
    updateInfo();
}

void ResearchTrendAnalyzer::saveSettings() {
    settings_.beginWriteArray("years");
    for (int i = 0; i < years_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("year", years_[i].year);
        QMap<QString, QVariant> map;
        for (auto it = years_[i].topicCounts.begin(); it != years_[i].topicCounts.end(); ++it) {
            map[it.key()] = it.value();
        }
        settings_.setValue("topics", map);
    }
    settings_.endArray();
}
