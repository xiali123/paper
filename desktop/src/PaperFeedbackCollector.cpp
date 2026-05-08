#include "PaperFeedbackCollector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperFeedbackCollector::PaperFeedbackCollector(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FeedbackCollector")
{
    setupUI();
    loadSettings();
}

void PaperFeedbackCollector::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Quality", "Clarity", "Novelty", "Methodology", "Relevance"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperFeedbackCollector::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    addBtn_ = new QPushButton("Add Feedback");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperFeedbackCollector::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFeedbackCollector::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Collect paper feedback");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 450);
}

void PaperFeedbackCollector::addFeedback(const FeedbackEntry& entry) {
    feedbacks_.append(entry);
    saveSettings();
    updateInfo();
    emit feedbackAdded(entry.paperId, entry.rating);
    emit feedbackSummary(feedbacks_.size(), averageRating());
    update();
}

QList<FeedbackEntry> PaperFeedbackCollector::feedbacks() const { return feedbacks_; }

qreal PaperFeedbackCollector::averageRating() const {
    if (feedbacks_.isEmpty()) return 0;
    int sum = 0;
    for (const auto& f : feedbacks_) sum += f.rating;
    return static_cast<qreal>(sum) / feedbacks_.size();
}

QMap<QString, qreal> PaperFeedbackCollector::ratingsByCategory() const {
    QMap<QString, qreal> sums;
    QMap<QString, int> counts;
    for (const auto& f : feedbacks_) {
        sums[f.category] += f.rating;
        counts[f.category]++;
    }
    QMap<QString, qreal> avgs;
    for (auto it = sums.begin(); it != sums.end(); ++it) {
        avgs[it.key()] = counts[it.key()] > 0 ? it.value() / counts[it.key()] : 0;
    }
    return avgs;
}

int PaperFeedbackCollector::feedbackCount() const { return feedbacks_.size(); }

void PaperFeedbackCollector::onAdd() {
    bool ok;
    QString paper = QInputDialog::getText(this, "Add Feedback", "Paper title:", QLineEdit::Normal, "", &ok);
    if (!ok || paper.isEmpty()) return;
    QStringList cats = {"quality", "clarity", "novelty", "methodology", "relevance"};
    QString cat = QInputDialog::getItem(this, "Add Feedback", "Category:", cats, 0, false, &ok);
    if (!ok) return;
    int rating = QInputDialog::getInt(this, "Add Feedback", "Rating (1-5):", 4, 1, 5, 1, &ok);
    if (!ok) return;
    QString comment = QInputDialog::getText(this, "Add Feedback", "Comment:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    FeedbackEntry f;
    f.id = feedbacks_.size() + 1;
    f.paperTitle = paper;
    f.category = cat;
    f.rating = rating;
    f.comment = comment;
    f.reviewer = "You";
    f.date = QDate::currentDate();
    addFeedback(f);
}

void PaperFeedbackCollector::onFilterChanged(int) { update(); }
void PaperFeedbackCollector::onClear() {
    feedbacks_.clear();
    saveSettings();
    infoLabel_->setText("Collect paper feedback");
    update();
}

void PaperFeedbackCollector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (feedbacks_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Collect paper feedback");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Feedback Collector");

    int w = width(), h = height();
    drawRatingChart(p, QRect(20, 50, w / 2 - 20, h / 2 - 40));
    drawCategoryRadar(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 40));
    drawRecentList(p, QRect(20, h / 2 + 20, w - 40, h / 2 - 50));
}

void PaperFeedbackCollector::drawRatingChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Rating Distribution");

    QMap<int, int> dist;
    for (const auto& f : feedbacks_) dist[f.rating]++;

    int maxVal = 1;
    for (const auto& v : dist) maxVal = qMax(maxVal, v);

    QColor starColors[] = {QColor(239,68,68), QColor(245,158,11), QColor(234,179,8),
                           QColor(16,185,129), QColor(59,130,246)};
    int barW = (rect.width() - 40) / 5;

    for (int i = 0; i < 5; ++i) {
        int x = rect.x() + 10 + i * barW;
        int count = dist.contains(i + 1) ? dist[i + 1] : 0;
        qreal h = (static_cast<qreal>(count) / maxVal) * (rect.height() - 55);

        p.setPen(Qt::NoPen);
        p.setBrush(starColors[i]);
        p.drawRoundedRect(x + 4, rect.bottom() - 25 - static_cast<int>(h), barW - 8, static_cast<int>(h), 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x, rect.bottom() - 8, barW, 14, Qt::AlignCenter,
                   QString::number(i + 1) + " star");
    }
}

void PaperFeedbackCollector::drawCategoryRadar(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Ratings");

    auto ratings = ratingsByCategory();
    QList<QString> cats = ratings.keys();
    if (cats.isEmpty()) return;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2 + 5;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;
    int n = cats.size();

    // Grid
    for (int r = 1; r <= 5; ++r) {
        int rr = radius * r / 5;
        p.setPen(QPen(QColor(241, 245, 249), 1));
        p.drawEllipse(QPoint(cx, cy), rr, rr);
    }

    QPolygonF polygon;
    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        qreal val = ratings[cats[i]] / 5.0;
        qreal px = cx + val * radius * std::cos(angle);
        qreal py = cy + val * radius * std::sin(angle);
        polygon << QPointF(px, py);

        qreal lx = cx + (radius + 15) * std::cos(angle);
        qreal ly = cy + (radius + 15) * std::sin(angle);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(QPointF(lx - 20, ly), cats[i].left(8));
    }

    p.setPen(QPen(QColor(59, 130, 246), 2));
    p.setBrush(QColor(59, 130, 246, 30));
    p.drawPolygon(polygon);

    p.setBrush(QColor(59, 130, 246));
    for (const auto& pt : polygon) p.drawEllipse(pt, 3, 3);
}

void PaperFeedbackCollector::drawRecentList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Recent Feedback");

    int filterIdx = filterCombo_->currentIndex();
    QStringList catKeys = {"quality", "clarity", "novelty", "methodology", "relevance"};
    QString filterCat = filterIdx > 0 ? catKeys[filterIdx - 1] : "";

    int itemH = qMin(28, (rect.height() - 25) / qMin(8, feedbacks_.size()));
    int count = 0;

    for (int i = feedbacks_.size() - 1; i >= 0 && count < 8; --i) {
        const auto& f = feedbacks_[i];
        if (!filterCat.isEmpty() && f.category != filterCat) continue;

        int y = rect.y() + 20 + count * itemH;

        // Stars
        p.setPen(QColor(245, 158, 11));
        p.setFont(QFont("Arial", 8));
        QString stars;
        for (int s = 0; s < f.rating; ++s) stars += "*";
        p.drawText(rect.x(), y, 40, itemH, Qt::AlignVCenter, stars);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 42, y, rect.width() / 2 - 50, itemH, Qt::AlignVCenter,
                   f.paperTitle.left(20));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y, rect.width() / 4, itemH, Qt::AlignVCenter,
                   f.category);

        p.drawText(rect.x() + rect.width() * 3 / 4, y, rect.width() / 4, itemH,
                   Qt::AlignVCenter | Qt::AlignRight, f.date.toString("MM/dd"));

        count++;
    }
}

void PaperFeedbackCollector::updateInfo() {
    if (feedbacks_.isEmpty()) { infoLabel_->setText("Collect paper feedback"); return; }
    infoLabel_->setText(QString("%1 feedbacks | Avg: %2/5 | Categories: %3")
        .arg(feedbacks_.size())
        .arg(averageRating(), 0, 'f', 1)
        .arg(ratingsByCategory().size()));
}

void PaperFeedbackCollector::loadSettings() {
    int size = settings_.beginReadArray("feedbacks");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FeedbackEntry f;
        f.id = settings_.value("id").toInt();
        f.paperId = settings_.value("paperId").toInt();
        f.paperTitle = settings_.value("paperTitle").toString();
        f.reviewer = settings_.value("reviewer").toString();
        f.category = settings_.value("category").toString();
        f.rating = settings_.value("rating").toInt();
        f.comment = settings_.value("comment").toString();
        f.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        feedbacks_.append(f);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFeedbackCollector::saveSettings() {
    settings_.beginWriteArray("feedbacks");
    for (int i = 0; i < feedbacks_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", feedbacks_[i].id);
        settings_.setValue("paperId", feedbacks_[i].paperId);
        settings_.setValue("paperTitle", feedbacks_[i].paperTitle);
        settings_.setValue("reviewer", feedbacks_[i].reviewer);
        settings_.setValue("category", feedbacks_[i].category);
        settings_.setValue("rating", feedbacks_[i].rating);
        settings_.setValue("comment", feedbacks_[i].comment);
        settings_.setValue("date", feedbacks_[i].date.toString(Qt::ISODate));
    }
    settings_.endArray();
}
