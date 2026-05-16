#include "visualization/PaperTrendPredictor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperTrendPredictor::PaperTrendPredictor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TrendPredictor")
{
    setupUI();
    loadSettings();
}

void PaperTrendPredictor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    predictBtn_ = new QPushButton("Predict");
    predictBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(predictBtn_, &QPushButton::clicked, this, &PaperTrendPredictor::onPredict);
    toolbar->addWidget(predictBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTrendPredictor::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter research topic to predict trends...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Predict research trends");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperTrendPredictor::addTrend(const TrendEntry& trend) {
    trends_.append(trend);
    saveSettings();
    updateInfo();
    emit trendPredicted(trend.id, trend.direction);
    update();
}

QList<TrendEntry> PaperTrendPredictor::trends() const { return trends_; }

QMap<QString, int> PaperTrendPredictor::directionCounts() const {
    QMap<QString, int> counts;
    for (const auto& t : trends_) counts[t.direction]++;
    return counts;
}

qreal PaperTrendPredictor::avgConfidence() const {
    if (trends_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& t : trends_) sum += t.confidence;
    return sum / trends_.size();
}

int PaperTrendPredictor::upTrends() const {
    int c = 0;
    for (const auto& t : trends_) if (t.direction == "up") c++;
    return c;
}

void PaperTrendPredictor::onPredict() {
    QString topic = inputField_->text().trimmed();
    if (topic.isEmpty()) return;

    QStringList directions = {"up", "down", "stable"};
    QColor dirColors[] = {QColor(16,185,129), QColor(239,68,68), QColor(59,130,246)};
    QStringList timeframes = {"1 year", "3 years", "5 years"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        TrendEntry t;
        t.id = trends_.size() + 1;
        t.topic = topic.left(10) + " Sub" + QString::number(i + 1);
        t.currentValue = 10 + QRandomGenerator::global()->bounded(90);
        int dIdx = QRandomGenerator::global()->bounded(directions.size());
        t.direction = directions[dIdx];
        qreal delta = (QRandomGenerator::global()->bounded(40) - 15) / 10.0;
        t.predictedValue = t.currentValue + delta;
        t.confidence = 0.4 + QRandomGenerator::global()->bounded(55) / 100.0;
        t.dataPoints = 10 + QRandomGenerator::global()->bounded(50);
        t.timeframe = timeframes[QRandomGenerator::global()->bounded(timeframes.size())];
        t.color = dirColors[dIdx];
        addTrend(t);
    }
    inputField_->clear();
}

void PaperTrendPredictor::onClear() {
    trends_.clear();
    saveSettings();
    infoLabel_->setText("Predict research trends");
    update();
}

void PaperTrendPredictor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (trends_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Predict research trends");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Trend Predictor");

    int w = width(), h = height();
    drawTrendList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDirectionChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTrendPredictor::drawTrendList(QPainter& p, const QRect& rect) {
    int show = qMin(10, trends_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    qreal maxVal = 1;
    for (const auto& t : trends_) maxVal = qMax(maxVal, qMax(t.currentValue, t.predictedValue));

    for (int i = 0; i < show; ++i) {
        const auto& t = trends_[i];
        int y = rect.y() + i * (itemH + 3);
        int barW = static_cast<int>((t.currentValue / maxVal) * (rect.width() / 2 - 40));
        int predW = static_cast<int>((t.predictedValue / maxVal) * (rect.width() / 2 - 40));

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setBrush(QColor(148, 163, 184));
        p.drawRoundedRect(rect.x() + 4, y + 3, barW, itemH / 2 - 2, 2, 2);

        p.setBrush(t.color);
        p.drawRoundedRect(rect.x() + 4, y + itemH / 2 + 1, predW, itemH / 2 - 3, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + barW + 8, y + 3, rect.width() / 2 - barW, itemH / 2, Qt::AlignVCenter,
                   t.topic.left(12));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(rect.x() + predW + 8, y + itemH / 2, rect.width() / 2 - predW, itemH / 2, Qt::AlignVCenter,
                   t.direction + " " + QString::number(t.confidence * 100, 'f', 0) + "%");

        p.drawText(rect.x() + rect.width() / 2 + 4, y + 4, rect.width() / 2 - 8, itemH - 4,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(t.currentValue, 'f', 1) + " -> " + QString::number(t.predictedValue, 'f', 1));
    }
}

void PaperTrendPredictor::drawDirectionChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Directions");

    auto counts = directionCounts();
    int total = trends_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 20 + pieW / 2;

    QStringList dirs = {"up", "stable", "down"};
    QColor colors[] = {QColor(16,185,129), QColor(59,130,246), QColor(239,68,68)};

    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(dirs[i]) ? counts[dirs[i]] : 0;
        qreal span = (static_cast<qreal>(count) / total) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperTrendPredictor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Trends", QString::number(trends_.size()), QColor(59,130,246)},
        {"Up", QString::number(upTrends()), QColor(16,185,129)},
        {"Confidence", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Down", QString::number(directionCounts().value("down", 0)), QColor(239,68,68)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTrendPredictor::updateInfo() {
    if (trends_.isEmpty()) { infoLabel_->setText("Predict research trends"); return; }
    infoLabel_->setText(QString("%1 trends | %2 up | %3% conf")
        .arg(trends_.size()).arg(upTrends()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperTrendPredictor::loadSettings() {
    int size = settings_.beginReadArray("trends");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TrendEntry t;
        t.id = settings_.value("id").toInt();
        t.topic = settings_.value("topic").toString();
        t.currentValue = settings_.value("currentValue").toDouble();
        t.predictedValue = settings_.value("predictedValue").toDouble();
        t.confidence = settings_.value("confidence").toDouble();
        t.direction = settings_.value("direction").toString();
        t.dataPoints = settings_.value("dataPoints").toInt();
        t.timeframe = settings_.value("timeframe").toString();
        t.color = QColor(settings_.value("color").toString());
        trends_.append(t);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTrendPredictor::saveSettings() {
    settings_.beginWriteArray("trends");
    for (int i = 0; i < trends_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", trends_[i].id);
        settings_.setValue("topic", trends_[i].topic);
        settings_.setValue("currentValue", trends_[i].currentValue);
        settings_.setValue("predictedValue", trends_[i].predictedValue);
        settings_.setValue("confidence", trends_[i].confidence);
        settings_.setValue("direction", trends_[i].direction);
        settings_.setValue("dataPoints", trends_[i].dataPoints);
        settings_.setValue("timeframe", trends_[i].timeframe);
        settings_.setValue("color", trends_[i].color.name());
    }
    settings_.endArray();
}
