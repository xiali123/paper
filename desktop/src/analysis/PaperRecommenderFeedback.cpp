#include "analysis/PaperRecommenderFeedback.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperRecommenderFeedback::PaperRecommenderFeedback(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RecommenderFeedback")
{
    setupUI();
    loadSettings();
}

void PaperRecommenderFeedback::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Feedback");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperRecommenderFeedback::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Accepted", "Rejected"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRecommenderFeedback::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title for feedback...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track recommendation feedback");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperRecommenderFeedback::addFeedback(const FeedbackEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit feedbackRecorded(entry.id, entry.accepted);
    update();
}

QList<FeedbackEntry> PaperRecommenderFeedback::entries() const { return entries_; }

int PaperRecommenderFeedback::acceptedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.accepted) c++;
    return c;
}

qreal PaperRecommenderFeedback::avgRating() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.rating;
    return sum / entries_.size();
}

QMap<QString, int> PaperRecommenderFeedback::sourceCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.source]++;
    return counts;
}

void PaperRecommenderFeedback::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList sources = {"collaborative", "content-based", "citation-based", "hybrid"};
    QStringList reasons = {"Relevant topic", "Good methodology", "Not relevant", "Already read", "High impact"};
    bool accepted = QRandomGenerator::global()->bounded(2) == 0;

    FeedbackEntry e;
    e.id = entries_.size() + 1;
    e.paperTitle = text.left(20);
    e.recommendation = "Paper-" + QString::number(e.id);
    e.feedback = accepted ? "Useful" : "Not useful";
    e.rating = 1 + QRandomGenerator::global()->bounded(50) / 10.0;
    e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
    e.accepted = accepted;
    e.reason = reasons[QRandomGenerator::global()->bounded(reasons.size())];
    e.color = accepted ? QColor(16,185,129) : QColor(239,68,68);
    addFeedback(e);
    inputField_->clear();
}

void PaperRecommenderFeedback::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track recommendation feedback");
    update();
}

void PaperRecommenderFeedback::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track recommendation feedback");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Recommender Feedback");

    int w = width(), h = height();
    drawFeedbackList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawAcceptChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRecommenderFeedback::drawFeedbackList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && !e.accepted) continue;
        if (filterIdx == 2 && e.accepted) continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   (e.accepted ? QString("+ ") : QString("- ")) + e.paperTitle.left(16));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.source + " | " + e.reason.left(12));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.rating, 'f', 1) + "/5.0");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, e.feedback);
        show++;
    }
}

void PaperRecommenderFeedback::drawAcceptChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Accept/Reject");

    int accepted = acceptedCount();
    int rejected = entries_.size() - accepted;
    int total = entries_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal acceptAngle = (static_cast<qreal>(accepted) / total) * 360;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(16,185,129));
    p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW, 0, static_cast<int>(acceptAngle * 16));

    p.setBrush(QColor(239,68,68));
    p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW, static_cast<int>(acceptAngle * 16),
              static_cast<int>((360 - acceptAngle) * 16));

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 15, cy + 5, QString::number(accepted) + "/" + QString::number(total));
}

void PaperRecommenderFeedback::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Feedback", QString::number(entries_.size()), QColor(59,130,246)},
        {"Accepted", QString::number(acceptedCount()), QColor(16,185,129)},
        {"Avg Rating", QString::number(avgRating(), 'f', 1) + "/5", QColor(245,158,11)},
        {"Sources", QString::number(sourceCounts().size()), QColor(139,92,246)}
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

void PaperRecommenderFeedback::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track recommendation feedback"); return; }
    infoLabel_->setText(QString("%1 feedback | %2 accepted | %3/5 avg")
        .arg(entries_.size()).arg(acceptedCount()).arg(avgRating(), 0, 'f', 1));
}

void PaperRecommenderFeedback::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FeedbackEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.recommendation = settings_.value("recommendation").toString();
        e.feedback = settings_.value("feedback").toString();
        e.rating = settings_.value("rating").toDouble();
        e.source = settings_.value("source").toString();
        e.accepted = settings_.value("accepted").toBool();
        e.reason = settings_.value("reason").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRecommenderFeedback::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("recommendation", entries_[i].recommendation);
        settings_.setValue("feedback", entries_[i].feedback);
        settings_.setValue("rating", entries_[i].rating);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("accepted", entries_[i].accepted);
        settings_.setValue("reason", entries_[i].reason);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
