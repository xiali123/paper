#include "citation/PaperPeerReviewTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperPeerReviewTracker::PaperPeerReviewTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PeerReviewTracker")
{
    setupUI();
    loadSettings();
}

void PaperPeerReviewTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Review");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperPeerReviewTracker::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPeerReviewTracker::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track peer reviews");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperPeerReviewTracker::addReview(const ReviewEntry& review) {
    reviews_.append(review);
    saveSettings();
    updateInfo();
    emit reviewAdded(review.id, review.recommendation);
    update();
}

QList<ReviewEntry> PaperPeerReviewTracker::reviews() const { return reviews_; }

QMap<QString, int> PaperPeerReviewTracker::recommendationCounts() const {
    QMap<QString, int> counts;
    for (const auto& r : reviews_) counts[r.recommendation]++;
    return counts;
}

qreal PaperPeerReviewTracker::avgQuality() const {
    if (reviews_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& r : reviews_) sum += r.quality;
    return sum / reviews_.size();
}

int PaperPeerReviewTracker::acceptCount() const {
    int c = 0;
    for (const auto& r : reviews_) if (r.recommendation == "accept") c++;
    return c;
}

void PaperPeerReviewTracker::onAdd() {
    bool ok;
    QString paper = QInputDialog::getText(this, "Add Review", "Paper:", QLineEdit::Normal, "", &ok);
    if (!ok || paper.isEmpty()) return;
    QString reviewer = QInputDialog::getText(this, "Add Review", "Reviewer:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QStringList recs = {"accept", "minor-revision", "major-revision", "reject"};
    QString rec = QInputDialog::getItem(this, "Add Review", "Recommendation:", recs, 1, false, &ok);
    if (!ok) return;

    ReviewEntry r;
    r.id = reviews_.size() + 1;
    r.paperTitle = paper;
    r.reviewer = reviewer.isEmpty() ? "Reviewer " + QString::number(r.id) : reviewer;
    r.recommendation = rec;
    r.quality = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    r.clarity = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    r.novelty = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    r.significance = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    r.comments = "Review for " + paper.left(12);

    QColor recColors[] = {QColor(16,185,129), QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};
    int rIdx = recs.indexOf(rec);
    r.color = recColors[qBound(0, rIdx, 3)];
    addReview(r);
}

void PaperPeerReviewTracker::onClear() {
    reviews_.clear();
    saveSettings();
    infoLabel_->setText("Track peer reviews");
    update();
}

void PaperPeerReviewTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (reviews_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track peer reviews");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Peer Review Tracker");

    int w = width(), h = height();
    drawReviewCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawScoreRadar(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 30, h / 2 - 40));
}

void PaperPeerReviewTracker::drawReviewCards(QPainter& p, const QRect& rect) {
    int show = qMin(8, reviews_.size());
    int cardH = qMin(44, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& r = reviews_[i];
        int y = rect.y() + i * (cardH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(r.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(r.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   r.paperTitle.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   r.reviewer.left(14) + " | " + r.recommendation);
        p.drawText(rect.x() + 10, y + 34, rect.width() / 2 - 10, 10, Qt::AlignVCenter,
                   "Q:" + QString::number(r.quality * 100, 'f', 0) + "% N:" + QString::number(r.novelty * 100, 'f', 0) + "%");

        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number((r.quality + r.clarity + r.novelty + r.significance) / 4 * 100, 'f', 0) + "% avg");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, r.comments.left(18));
    }
}

void PaperPeerReviewTracker::drawScoreRadar(QPainter& p, const QRect& rect) {
    if (reviews_.isEmpty()) return;
    const auto& r = reviews_.last();

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 25;
    int n = 4;
    QStringList labels = {"Quality", "Clarity", "Novelty", "Significance"};
    qreal values[] = {r.quality, r.clarity, r.novelty, r.significance};

    for (int ring = 1; ring <= 4; ++ring) {
        int rr = radius * ring / 4;
        p.setPen(QPen(QColor(241, 245, 249), 1));
        p.drawEllipse(QPoint(cx, cy), rr, rr);
    }

    QPolygonF polygon;
    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        qreal px = cx + values[i] * radius * std::cos(angle);
        qreal py = cy + values[i] * radius * std::sin(angle);
        polygon << QPointF(px, py);

        qreal lx = cx + (radius + 16) * std::cos(angle);
        qreal ly = cy + (radius + 16) * std::sin(angle);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(QPointF(lx - 25, ly + 4), labels[i]);
    }

    p.setPen(QPen(r.color, 2));
    p.setBrush(QColor(r.color.red(), r.color.green(), r.color.blue(), 40));
    p.drawPolygon(polygon);

    p.setBrush(r.color);
    for (const auto& pt : polygon) p.drawEllipse(pt, 3, 3);
}

void PaperPeerReviewTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Reviews", QString::number(reviews_.size()), QColor(59,130,246)},
        {"Accepted", QString::number(acceptCount()), QColor(16,185,129)},
        {"Avg Quality", QString::number(avgQuality() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Rejected", QString::number(recommendationCounts().value("reject", 0)), QColor(239,68,68)}
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

void PaperPeerReviewTracker::updateInfo() {
    if (reviews_.isEmpty()) { infoLabel_->setText("Track peer reviews"); return; }
    infoLabel_->setText(QString("%1 reviews | %2 accepted | avg quality: %3%")
        .arg(reviews_.size()).arg(acceptCount()).arg(avgQuality() * 100, 0, 'f', 0));
}

void PaperPeerReviewTracker::loadSettings() {
    int size = settings_.beginReadArray("reviews");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReviewEntry r;
        r.id = settings_.value("id").toInt();
        r.paperTitle = settings_.value("paperTitle").toString();
        r.reviewer = settings_.value("reviewer").toString();
        r.recommendation = settings_.value("recommendation").toString();
        r.quality = settings_.value("quality").toDouble();
        r.clarity = settings_.value("clarity").toDouble();
        r.novelty = settings_.value("novelty").toDouble();
        r.significance = settings_.value("significance").toDouble();
        r.comments = settings_.value("comments").toString();
        r.color = QColor(settings_.value("color").toString());
        reviews_.append(r);
    }
    settings_.endArray();
    updateInfo();
}

void PaperPeerReviewTracker::saveSettings() {
    settings_.beginWriteArray("reviews");
    for (int i = 0; i < reviews_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", reviews_[i].id);
        settings_.setValue("paperTitle", reviews_[i].paperTitle);
        settings_.setValue("reviewer", reviews_[i].reviewer);
        settings_.setValue("recommendation", reviews_[i].recommendation);
        settings_.setValue("quality", reviews_[i].quality);
        settings_.setValue("clarity", reviews_[i].clarity);
        settings_.setValue("novelty", reviews_[i].novelty);
        settings_.setValue("significance", reviews_[i].significance);
        settings_.setValue("comments", reviews_[i].comments);
        settings_.setValue("color", reviews_[i].color.name());
    }
    settings_.endArray();
}
