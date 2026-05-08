#include "visualization/PaperRatingChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include <cmath>

PaperRatingChart::PaperRatingChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PaperRatings")
{
    setupUI();
    loadSettings();
}

void PaperRatingChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Chart:"));
    chartTypeCombo_ = new QComboBox();
    chartTypeCombo_->addItems({"Radar", "Bar", "Scatter"});
    connect(chartTypeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperRatingChart::onChartTypeChanged);
    toolbar->addWidget(chartTypeCombo_, 1);

    toolbar->addWidget(new QLabel("Sort:"));
    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Overall", "Novelty", "Methodology", "Recent"});
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperRatingChart::onSortChanged);
    toolbar->addWidget(sortCombo_, 1);

    sampleBtn_ = new QPushButton("Add Sample");
    sampleBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(sampleBtn_, &QPushButton::clicked, this, &PaperRatingChart::onAddSample);
    toolbar->addWidget(sampleBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRatingChart::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    statsLabel_ = new QLabel("No ratings");
    statsLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(statsLabel_);

    setMinimumSize(500, 400);
}

void PaperRatingChart::setPaper(int paperId, const QString& title) {
    Q_UNUSED(paperId); Q_UNUSED(title);
    update();
}

void PaperRatingChart::addRating(const RatingEntry& rating) {
    bool updated = false;
    for (auto& r : ratings_) {
        if (r.paperId == rating.paperId) {
            r = rating;
            updated = true;
            emit ratingUpdated(r.paperId, r.overall);
            break;
        }
    }
    if (!updated) {
        ratings_.append(rating);
        emit ratingAdded(rating.paperId, rating.overall);
    }
    saveSettings();
    updateStats();
    update();
}

void PaperRatingChart::removeRating(int paperId) {
    ratings_.removeIf([paperId](const RatingEntry& r) { return r.paperId == paperId; });
    saveSettings();
    updateStats();
    update();
}

QList<RatingEntry> PaperRatingChart::ratings() const { return ratings_; }

RatingEntry PaperRatingChart::ratingForPaper(int paperId) const {
    for (const auto& r : ratings_) {
        if (r.paperId == paperId) return r;
    }
    return RatingEntry{};
}

void PaperRatingChart::onChartTypeChanged(int) { update(); }
void PaperRatingChart::onSortChanged(int) { update(); }

void PaperRatingChart::onAddSample() {
    static int sampleIdx = 0;
    QStringList titles = {"Attention Is All You Need", "BERT", "GPT-3", "ResNet", "Transformer-XL",
                          "ALBERT", "T5", "ViT", "CLIP", "DALL-E"};
    RatingEntry r;
    r.paperId = 100 + sampleIdx;
    r.title = titles[sampleIdx % titles.size()];
    r.novelty = 3 + QRandomGenerator::global()->bounded(70) / 10.0;
    r.methodology = 4 + QRandomGenerator::global()->bounded(60) / 10.0;
    r.clarity = 5 + QRandomGenerator::global()->bounded(50) / 10.0;
    r.significance = 3 + QRandomGenerator::global()->bounded(70) / 10.0;
    r.reproducibility = 4 + QRandomGenerator::global()->bounded(60) / 10.0;
    r.overall = (r.novelty + r.methodology + r.clarity + r.significance + r.reproducibility) / 5.0;
    addRating(r);
    sampleIdx++;
}

void PaperRatingChart::onClear() {
    ratings_.clear();
    saveSettings();
    updateStats();
    update();
}

void PaperRatingChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (ratings_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No ratings. Click 'Add Sample' to add data.");
        return;
    }

    int w = width();
    int h = height();
    QRect chartRect(40, 80, w - 80, h - 120);

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 40, "Paper Rating Chart");

    QString chartType = chartTypeCombo_->currentText();
    if (chartType == "Radar") {
        drawRadarChart(p, chartRect, ratings_.first());
    } else if (chartType == "Bar") {
        drawBarChart(p, chartRect);
    } else {
        drawScatterPlot(p, chartRect);
    }
}

void PaperRatingChart::drawRadarChart(QPainter& p, const QRect& rect, const RatingEntry& r) {
    QString labels[] = {"Novelty", "Methodology", "Clarity", "Significance", "Reproducibility"};
    qreal values[] = {r.novelty, r.methodology, r.clarity, r.significance, r.reproducibility};
    int n = 5;

    qreal cx = rect.center().x();
    qreal cy = rect.center().y();
    qreal maxR = qMin(rect.width(), rect.height()) / 2 - 30;

    // Grid rings
    p.setPen(QPen(QColor(226, 232, 240), 1));
    for (int ring = 1; ring <= 5; ++ring) {
        qreal r2 = maxR * ring / 5;
        QPolygonF poly;
        for (int i = 0; i < n; ++i) {
            qreal angle = -M_PI / 2 + 2 * M_PI * i / n;
            poly << QPointF(cx + r2 * std::cos(angle), cy + r2 * std::sin(angle));
        }
        p.drawPolygon(poly);
    }

    // Axes + labels
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.setFont(QFont("Arial", 9));
    for (int i = 0; i < n; ++i) {
        qreal angle = -M_PI / 2 + 2 * M_PI * i / n;
        QPointF edge(cx + maxR * std::cos(angle), cy + maxR * std::sin(angle));
        p.drawLine(QPointF(cx, cy), edge);
        QPointF labelPos(cx + (maxR + 15) * std::cos(angle), cy + (maxR + 15) * std::sin(angle));
        p.setPen(QColor(100, 116, 139));
        p.drawText(labelPos - QPoint(30, -5), 60, 20, Qt::AlignCenter,
                   QString("%1 (%2)").arg(labels[i]).arg(values[i], 0, 'f', 1));
        p.setPen(QPen(QColor(203, 213, 225), 1));
    }

    // Data polygon
    QPolygonF dataPoly;
    for (int i = 0; i < n; ++i) {
        qreal angle = -M_PI / 2 + 2 * M_PI * i / n;
        qreal r2 = maxR * values[i] / 10.0;
        dataPoly << QPointF(cx + r2 * std::cos(angle), cy + r2 * std::sin(angle));
    }
    p.setPen(QPen(QColor(59, 130, 246), 2));
    p.setBrush(QColor(59, 130, 246, 50));
    p.drawPolygon(dataPoly);

    // Dots
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(59, 130, 246));
    for (const auto& pt : dataPoly) p.drawEllipse(pt, 4, 4);

    // Overall score
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.bottomRight() - QPoint(100, 5), QString("Overall: %1").arg(r.overall, 0, 'f', 1));
}

void PaperRatingChart::drawBarChart(QPainter& p, const QRect& rect) {
    QList<RatingEntry> sorted = ratings_;
    int sortIdx = sortCombo_->currentIndex();
    if (sortIdx == 0) std::sort(sorted.begin(), sorted.end(),
        [](const RatingEntry& a, const RatingEntry& b) { return a.overall > b.overall; });
    else if (sortIdx == 1) std::sort(sorted.begin(), sorted.end(),
        [](const RatingEntry& a, const RatingEntry& b) { return a.novelty > b.novelty; });
    else if (sortIdx == 2) std::sort(sorted.begin(), sorted.end(),
        [](const RatingEntry& a, const RatingEntry& b) { return a.methodology > b.methodology; });

    int n = qMin(sorted.size(), 15);
    int barH = qMin(25, (rect.height() - 20) / n);
    qreal maxVal = 10.0;

    for (int i = 0; i < n; ++i) {
        int y = rect.y() + i * (barH + 4);
        qreal w = (sorted[i].overall / maxVal) * (rect.width() - 120);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 6, 100, 20, Qt::AlignRight | Qt::AlignVCenter,
                   sorted[i].title.left(14));

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x() + 105, y, rect.width() - 120, barH - 2, 3, 3);

        // Bar fill
        QColor color = sorted[i].overall >= 8 ? QColor(16, 185, 129) :
                       sorted[i].overall >= 6 ? QColor(59, 130, 246) :
                       sorted[i].overall >= 4 ? QColor(245, 158, 11) : QColor(239, 68, 68);
        p.setBrush(color);
        p.drawRoundedRect(rect.x() + 105, y, static_cast<int>(w), barH - 2, 3, 3);

        // Score
        p.setPen(QColor(15, 23, 42));
        p.drawText(rect.x() + 108 + static_cast<int>(w), y + barH - 6,
                   QString::number(sorted[i].overall, 'f', 1));
    }
}

void PaperRatingChart::drawScatterPlot(QPainter& p, const QRect& rect) {
    // X = novelty, Y = significance, size = overall
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.drawLine(rect.bottomLeft(), rect.bottomRight());
    p.drawLine(rect.bottomLeft(), rect.topLeft());

    p.setFont(QFont("Arial", 9));
    p.setPen(QColor(100, 116, 139));
    p.drawText(rect.bottomRight() + QPoint(-60, 15), "Novelty ->");
    p.drawText(rect.topLeft() - QPoint(35, -5), "Significance");

    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153)};

    for (int i = 0; i < ratings_.size(); ++i) {
        const auto& r = ratings_[i];
        qreal x = rect.x() + (r.novelty / 10.0) * rect.width();
        qreal y = rect.bottom() - (r.significance / 10.0) * rect.height();
        qreal radius = 4 + r.overall;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i % 6]);
        p.drawEllipse(QPointF(x, y), radius, radius);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(QPointF(x + radius + 2, y + 4), r.title.left(12));
    }
}

void PaperRatingChart::mousePressEvent(QMouseEvent* event) {
    QWidget::mousePressEvent(event);
}

void PaperRatingChart::updateStats() {
    if (ratings_.isEmpty()) {
        statsLabel_->setText("No ratings");
        return;
    }
    qreal avg = 0;
    for (const auto& r : ratings_) avg += r.overall;
    avg /= ratings_.size();
    qreal maxO = 0;
    QString best;
    for (const auto& r : ratings_) {
        if (r.overall > maxO) { maxO = r.overall; best = r.title; }
    }
    statsLabel_->setText(QString("%1 papers rated | Avg: %2 | Best: %3 (%4)")
        .arg(ratings_.size()).arg(avg, 0, 'f', 1).arg(best.left(20)).arg(maxO, 0, 'f', 1));
}

void PaperRatingChart::loadSettings() {
    QByteArray data = settings_.value("ratings").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        RatingEntry r;
        r.paperId = obj["paperId"].toInt();
        r.title = obj["title"].toString();
        r.novelty = obj["novelty"].toDouble();
        r.methodology = obj["methodology"].toDouble();
        r.clarity = obj["clarity"].toDouble();
        r.significance = obj["significance"].toDouble();
        r.reproducibility = obj["reproducibility"].toDouble();
        r.overall = obj["overall"].toDouble();
        r.notes = obj["notes"].toString();
        ratings_.append(r);
    }
    updateStats();
}

void PaperRatingChart::saveSettings() {
    QJsonArray arr;
    for (const auto& r : ratings_) {
        QJsonObject obj;
        obj["paperId"] = r.paperId;
        obj["title"] = r.title;
        obj["novelty"] = r.novelty;
        obj["methodology"] = r.methodology;
        obj["clarity"] = r.clarity;
        obj["significance"] = r.significance;
        obj["reproducibility"] = r.reproducibility;
        obj["overall"] = r.overall;
        obj["notes"] = r.notes;
        arr.append(obj);
    }
    settings_.setValue("ratings", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
