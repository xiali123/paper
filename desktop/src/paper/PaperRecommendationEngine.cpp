#include "paper/PaperRecommendationEngine.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <algorithm>

PaperRecommendationEngine::PaperRecommendationEngine(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperRecommendationEngine::setupUI() {
    auto* layout = new QVBoxLayout(this);

    statsLabel_ = new QLabel("Recommendations");
    statsLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(statsLabel_);

    recList_ = new QListWidget();
    recList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid palette(mid); }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    recList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    recList_->setResizeMode(QListWidget::Adjust);
    layout->addWidget(recList_, 1);

    auto* btnRow = new QHBoxLayout();
    auto* refreshBtn = new QPushButton("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(refreshBtn, &QPushButton::clicked, this, &PaperRecommendationEngine::onRefresh);
    btnRow->addWidget(refreshBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);
}

void PaperRecommendationEngine::setUserHistory(const QList<int>& viewedPaperIds) {
    viewedIds_ = QSet<int>(viewedPaperIds.begin(), viewedPaperIds.end());
}

void PaperRecommendationEngine::setUserFavorites(const QList<int>& favoritePaperIds) {
    favoriteIds_ = QSet<int>(favoritePaperIds.begin(), favoritePaperIds.end());
}

void PaperRecommendationEngine::setUserKeywords(const QStringList& keywords) {
    userKeywords_ = keywords;
}

void PaperRecommendationEngine::setPaperPool(const QList<QPair<int, QString>>& papers) {
    paperPool_ = papers;
}

void PaperRecommendationEngine::setUserRatings(const QMap<int, int>& ratings) {
    userRatings_ = ratings;
}

QList<RecommendationScore> PaperRecommendationEngine::computeRecommendations(int maxResults) {
    QList<RecommendationScore> results;

    for (const auto& [paperId, keywords] : paperPool_) {
        // Skip already viewed
        if (viewedIds_.contains(paperId)) continue;

        RecommendationScore score;
        score.paperId = paperId;
        score.title = QString("Paper #%1").arg(paperId);

        // Keyword overlap score (0-1)
        score.score = computeKeywordScore(keywords);

        // Collaborative boost
        score.score += computeCollaborativeScore(paperId) * 0.3;

        // Favorite boost
        if (favoriteIds_.contains(paperId)) score.score += 0.2;

        // Rating boost
        if (userRatings_.contains(paperId)) {
            score.score += userRatings_[paperId] * 0.05;
        }

        // Clamp
        score.score = qBound(0.0, score.score, 1.0);

        // Generate reason
        if (score.score >= 0.7) score.reason = "Highly relevant to your interests";
        else if (score.score >= 0.5) score.reason = "Matches your research topics";
        else if (score.score >= 0.3) score.reason = "Related to papers you've viewed";
        else score.reason = "May be of interest";

        if (score.score >= minScore_) {
            results.append(score);
        }
    }

    // Sort by score descending
    std::sort(results.begin(), results.end(),
              [](const RecommendationScore& a, const RecommendationScore& b) {
                  return a.score > b.score;
              });

    if (results.size() > maxResults) results.resize(maxResults);

    return results;
}

double PaperRecommendationEngine::computeKeywordScore(const QString& paperKeywords) const {
    if (userKeywords_.isEmpty() || paperKeywords.isEmpty()) return 0.0;

    QStringList pKws = paperKeywords.split(",", Qt::SkipEmptyParts);
    int matches = 0;
    for (const auto& pkw : pKws) {
        QString trimmed = pkw.trimmed().toLower();
        for (const auto& ukw : userKeywords_) {
            if (trimmed.contains(ukw.toLower()) || ukw.toLower().contains(trimmed)) {
                matches++;
                break;
            }
        }
    }

    return (pKws.isEmpty()) ? 0.0 : static_cast<double>(matches) / pKws.size();
}

double PaperRecommendationEngine::computeCollaborativeScore(int paperId) const {
    Q_UNUSED(paperId);
    // Placeholder: in real impl, compare with similar users' preferences
    return 0.0;
}

void PaperRecommendationEngine::onRefresh() {
    auto recs = computeRecommendations();
    displayRecommendations(recs);
    emit recommendationsReady(recs);
}

void PaperRecommendationEngine::displayRecommendations(const QList<RecommendationScore>& recs) {
    recList_->clear();

    if (recs.isEmpty()) {
        statsLabel_->setText("No recommendations available");
        return;
    }

    statsLabel_->setText(QString("%1 recommendation(s)").arg(recs.size()));

    for (const auto& rec : recs) {
        auto* item = new QListWidgetItem();

        // Score bar color
        QString color = rec.score >= 0.7 ? "#059669" :
                        rec.score >= 0.5 ? "#d97706" : "#64748b";

        QString text = QString("[%1%] %2\n%3")
            .arg(static_cast<int>(rec.score * 100))
            .arg(rec.title.left(60))
            .arg(rec.reason);

        item->setText(text);
        item->setData(Qt::UserRole, rec.paperId);
        item->setForeground(QColor(color));

        QFont font;
        font.setBold(rec.score >= 0.7);
        item->setFont(font);

        recList_->addItem(item);

        emit paperRecommended(rec.paperId, rec.score, rec.reason);
    }
}
