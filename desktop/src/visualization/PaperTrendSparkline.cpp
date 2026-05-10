#include "visualization/PaperTrendSparkline.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperTrendSparkline::PaperTrendSparkline(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TrendSparkline")
{
    setupUI();
    loadSettings();
}

void PaperTrendSparkline::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperTrendSparkline::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Weekly", "Monthly", "Quarterly"});
    toolbar->addWidget(periodCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTrendSparkline::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter topic for trend sparkline...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate trend sparklines");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperTrendSparkline::addEntry(const SparklineEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sparklineGenerated(entry.id, entry.trend);
    update();
}

QList<SparklineEntry> PaperTrendSparkline::entries() const { return entries_; }

qreal PaperTrendSparkline::avgTrend() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.trend;
    return sum / entries_.size();
}

int PaperTrendSparkline::risingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.direction == "rising") c++;
    return c;
}

QMap<QString, int> PaperTrendSparkline::directionCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.direction]++;
    return counts;
}

void PaperTrendSparkline::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList directions = {"rising", "stable", "declining"};
    QStringList periods = {"weekly", "monthly", "quarterly"};

    int pIdx = periodCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        SparklineEntry e;
        e.id = entries_.size() + 1;
        e.topic = text.left(10) + " T" + QString::number(i + 1);
        int pointCount = 8 + QRandomGenerator::global()->bounded(8);
        qreal base = 30 + QRandomGenerator::global()->bounded(50);
        e.values.clear();
        for (int j = 0; j < pointCount; ++j) {
            base += -5 + QRandomGenerator::global()->bounded(11);
            e.values << qBound(5.0, base, 100.0);
        }
        e.trend = e.values.last() - e.values.first();
        e.volatility = 0;
        for (int j = 1; j < e.values.size(); ++j)
            e.volatility += std::abs(e.values[j] - e.values[j-1]);
        e.volatility /= qMax(e.values.size() - 1, 1);
        e.direction = e.trend > 5 ? "rising" : (e.trend < -5 ? "declining" : "stable");
        e.period = periods[pIdx];
        e.peak = *std::max_element(e.values.begin(), e.values.end());
        e.trough = *std::min_element(e.values.begin(), e.values.end());
        e.color = e.direction == "rising" ? QColor(16,185,129) : (e.direction == "stable" ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTrendSparkline::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate trend sparklines");
    update();
}

void PaperTrendSparkline::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate trend sparklines");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Trend Sparklines");

    int w = width(), h = height();
    drawSparklineList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDirectionChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTrendSparkline::drawSparklineList(QPainter& p, const QRect& rect) {
    int show = qMin(8, entries_.size());
    int itemH = qMin(42, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.topic.left(12) + " [" + e.direction.left(4) + "]");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.period + " | vol: " + QString::number(e.volatility, 'f', 1));

        // Draw mini sparkline
        int sparkX = rect.x() + rect.width() / 2 + 5;
        int sparkW = rect.width() / 2 - 20;
        int sparkH = itemH - 8;
        if (e.values.size() >= 2) {
            qreal vMin = *std::min_element(e.values.begin(), e.values.end());
            qreal vMax = *std::max_element(e.values.begin(), e.values.end());
            qreal vRange = qMax(vMax - vMin, 1.0);

            QPolygonF poly;
            for (int j = 0; j < e.values.size(); ++j) {
                qreal px = sparkX + (static_cast<qreal>(j) / (e.values.size() - 1)) * sparkW;
                qreal py = y + itemH - 4 - ((e.values[j] - vMin) / vRange) * sparkH;
                poly << QPointF(px, py);
            }
            p.setPen(QPen(e.color, 2));
            p.setBrush(Qt::NoBrush);
            p.drawPolyline(poly);
        }

        p.setPen(e.color);
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   (e.trend >= 0 ? "+" : "") + QString::number(e.trend, 'f', 0));
    }
}

void PaperTrendSparkline::drawDirectionChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Directions");

    auto counts = directionCounts();
    QStringList dirs = {"rising", "stable", "declining"};
    QString labels[] = {"Rising", "Stable", "Declining"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(dirs[i]) ? counts[dirs[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
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

void PaperTrendSparkline::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Trends", QString::number(entries_.size()), QColor(59,130,246)},
        {"Rising", QString::number(risingCount()), QColor(16,185,129)},
        {"Avg Trend", QString::number(avgTrend(), 'f', 1), QColor(245,158,11)},
        {"Directions", QString::number(directionCounts().size()), QColor(139,92,246)}
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

void PaperTrendSparkline::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate trend sparklines"); return; }
    infoLabel_->setText(QString("%1 trends | %2 rising | %3 avg")
        .arg(entries_.size()).arg(risingCount()).arg(avgTrend(), 0, 'f', 1));
}

void PaperTrendSparkline::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SparklineEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        int valSize = settings_.beginReadArray("values");
        for (int j = 0; j < valSize; ++j) {
            settings_.setArrayIndex(j);
            e.values << settings_.value("v").toDouble();
        }
        settings_.endArray();
        e.trend = settings_.value("trend").toDouble();
        e.volatility = settings_.value("volatility").toDouble();
        e.direction = settings_.value("direction").toString();
        e.period = settings_.value("period").toString();
        e.peak = settings_.value("peak").toDouble();
        e.trough = settings_.value("trough").toDouble();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTrendSparkline::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.beginWriteArray("values");
        for (int j = 0; j < entries_[i].values.size(); ++j) {
            settings_.setArrayIndex(j);
            settings_.setValue("v", entries_[i].values[j]);
        }
        settings_.endArray();
        settings_.setValue("trend", entries_[i].trend);
        settings_.setValue("volatility", entries_[i].volatility);
        settings_.setValue("direction", entries_[i].direction);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("peak", entries_[i].peak);
        settings_.setValue("trough", entries_[i].trough);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
