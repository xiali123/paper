#include "paper/PaperReadingScoreWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperReadingScoreWidget::PaperReadingScoreWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingScore")
{
    setupUI();
    loadSettings();
}

void PaperReadingScoreWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    evalBtn_ = new QPushButton("Evaluate");
    evalBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(evalBtn_, &QPushButton::clicked, this, &PaperReadingScoreWidget::onEvaluate);
    toolbar->addWidget(evalBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingScoreWidget::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Evaluate reading quality");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 480);
}

void PaperReadingScoreWidget::addScore(const ReadingScore& score) {
    scores_.append(score);
    saveSettings();
    updateInfo();
    emit scoreUpdated(scores_.size(), averageOverall());
    update();
}

QList<ReadingScore> PaperReadingScoreWidget::scores() const { return scores_; }

qreal PaperReadingScoreWidget::averageOverall() const {
    if (scores_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& s : scores_) sum += s.overall;
    return sum / scores_.size();
}

ReadingScore PaperReadingScoreWidget::bestScore() const {
    if (scores_.isEmpty()) return ReadingScore();
    ReadingScore best = scores_.first();
    for (const auto& s : scores_) if (s.overall > best.overall) best = s;
    return best;
}

void PaperReadingScoreWidget::onEvaluate() {
    bool ok;
    QString title = QInputDialog::getText(this, "Evaluate", "Paper title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;

    ReadingScore s;
    s.paperId = scores_.size() + 1;
    s.paperTitle = title;
    s.comprehension = 0.4 + QRandomGenerator::global()->bounded(55) / 100.0;
    s.speed = 0.3 + QRandomGenerator::global()->bounded(65) / 100.0;
    s.retention = 0.4 + QRandomGenerator::global()->bounded(55) / 100.0;
    s.engagement = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
    s.overall = (s.comprehension + s.speed + s.retention + s.engagement) / 4;

    if (s.overall >= 0.85) { s.grade = "A"; s.color = QColor(16,185,129); }
    else if (s.overall >= 0.7) { s.grade = "B"; s.color = QColor(59,130,246); }
    else if (s.overall >= 0.55) { s.grade = "C"; s.color = QColor(245,158,11); }
    else { s.grade = "D"; s.color = QColor(239,68,68); }

    addScore(s);
}

void PaperReadingScoreWidget::onClear() {
    scores_.clear();
    saveSettings();
    infoLabel_->setText("Evaluate reading quality");
    update();
}

void PaperReadingScoreWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (scores_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Evaluate reading quality");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Score");

    int w = width(), h = height();
    drawRadarChart(p, QRect(20, 50, w / 2 - 20, h / 2));
    drawScoreBars(p, QRect(20, h / 2 + 10, w - 40, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawHistory(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 30, h / 2 - 30));
}

void PaperReadingScoreWidget::drawRadarChart(QPainter& p, const QRect& rect) {
    if (scores_.isEmpty()) return;
    const auto& s = scores_.last();

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2 + 5;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;
    int n = 4;
    QStringList labels = {"Comprehension", "Speed", "Retention", "Engagement"};
    qreal values[] = {s.comprehension, s.speed, s.retention, s.engagement};

    for (int r = 1; r <= 5; ++r) {
        int rr = radius * r / 5;
        p.setPen(QPen(QColor(241, 245, 249), 1));
        p.drawEllipse(QPoint(cx, cy), rr, rr);
    }

    QPolygonF polygon;
    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        qreal px = cx + values[i] * radius * std::cos(angle);
        qreal py = cy + values[i] * radius * std::sin(angle);
        polygon << QPointF(px, py);

        qreal lx = cx + (radius + 18) * std::cos(angle);
        qreal ly = cy + (radius + 18) * std::sin(angle);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(QPointF(lx - 25, ly + 4), labels[i]);
    }

    p.setPen(QPen(s.color, 2));
    p.setBrush(QColor(s.color.red(), s.color.green(), s.color.blue(), 40));
    p.drawPolygon(polygon);

    p.setBrush(s.color);
    for (const auto& pt : polygon) p.drawEllipse(pt, 4, 4);
}

void PaperReadingScoreWidget::drawScoreBars(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Score History");

    int show = qMin(12, scores_.size());
    int barH = qMin(16, (rect.height() - 25) / show);

    for (int i = 0; i < show; ++i) {
        const auto& s = scores_[scores_.size() - 1 - i];
        int y = rect.y() + 20 + i * (barH + 3);
        int barW = static_cast<int>(s.overall * (rect.width() - 140));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter,
                   s.paperTitle.left(12));

        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 2, s.grade);
    }
}

void PaperReadingScoreWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(scores_.size()), QColor(59,130,246)},
        {"Average", QString::number(averageOverall() * 100, 'f', 0) + "%", QColor(16,185,129)},
        {"Best", bestScore().grade, bestScore().color},
        {"Best Title", bestScore().paperTitle.left(12), QColor(139,92,246)}
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

void PaperReadingScoreWidget::drawHistory(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Grade Distribution");

    QMap<QString, int> dist;
    for (const auto& s : scores_) dist[s.grade]++;
    QStringList grades = {"A", "B", "C", "D"};
    QColor colors[] = {QColor(16,185,129), QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : dist) maxVal = qMax(maxVal, v);

    int barW = (rect.width() - 30) / 4;
    for (int i = 0; i < 4; ++i) {
        int x = rect.x() + 10 + i * barW;
        int count = dist.contains(grades[i]) ? dist[grades[i]] : 0;
        qreal h = (static_cast<qreal>(count) / maxVal) * (rect.height() - 55);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(x + 4, rect.bottom() - 25 - static_cast<int>(h), barW - 8, static_cast<int>(h), 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x, rect.bottom() - 8, barW, 14, Qt::AlignCenter, grades[i]);
    }
}

void PaperReadingScoreWidget::updateInfo() {
    if (scores_.isEmpty()) { infoLabel_->setText("Evaluate reading quality"); return; }
    infoLabel_->setText(QString("%1 papers | avg: %2% | best: %3")
        .arg(scores_.size()).arg(averageOverall() * 100, 0, 'f', 0).arg(bestScore().grade));
}

void PaperReadingScoreWidget::loadSettings() {
    int size = settings_.beginReadArray("scores");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingScore s;
        s.paperId = settings_.value("id").toInt();
        s.paperTitle = settings_.value("title").toString();
        s.comprehension = settings_.value("comprehension").toDouble();
        s.speed = settings_.value("speed").toDouble();
        s.retention = settings_.value("retention").toDouble();
        s.engagement = settings_.value("engagement").toDouble();
        s.overall = settings_.value("overall").toDouble();
        s.grade = settings_.value("grade").toString();
        s.color = QColor(settings_.value("color").toString());
        scores_.append(s);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingScoreWidget::saveSettings() {
    settings_.beginWriteArray("scores");
    for (int i = 0; i < scores_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", scores_[i].paperId);
        settings_.setValue("title", scores_[i].paperTitle);
        settings_.setValue("comprehension", scores_[i].comprehension);
        settings_.setValue("speed", scores_[i].speed);
        settings_.setValue("retention", scores_[i].retention);
        settings_.setValue("engagement", scores_[i].engagement);
        settings_.setValue("overall", scores_[i].overall);
        settings_.setValue("grade", scores_[i].grade);
        settings_.setValue("color", scores_[i].color.name());
    }
    settings_.endArray();
}
