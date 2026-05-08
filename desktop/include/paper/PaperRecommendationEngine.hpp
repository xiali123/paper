#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QMap>
#include <QPair>
#include <QSet>

struct RecommendationScore {
    int paperId{-1};
    QString title;
    double score{0.0};
    QString reason;
};

class PaperRecommendationEngine : public QWidget {
    Q_OBJECT

public:
    explicit PaperRecommendationEngine(QWidget* parent = nullptr);

    void setUserHistory(const QList<int>& viewedPaperIds);
    void setUserFavorites(const QList<int>& favoritePaperIds);
    void setUserKeywords(const QStringList& keywords);
    void setPaperPool(const QList<QPair<int, QString>>& papers); // id, keywords
    void setUserRatings(const QMap<int, int>& ratings); // paperId -> stars

    QList<RecommendationScore> computeRecommendations(int maxResults = 20);

    void setMinScore(double min) { minScore_ = min; }
    double minScore() const { return minScore_; }

signals:
    void recommendationsReady(const QList<RecommendationScore>& recs);
    void paperRecommended(int paperId, double score, const QString& reason);

private slots:
    void onRefresh();

private:
    void setupUI();
    void displayRecommendations(const QList<RecommendationScore>& recs);
    double computeKeywordScore(const QString& paperKeywords) const;
    double computeCollaborativeScore(int paperId) const;

    QListWidget* recList_{nullptr};
    QLabel* statsLabel_{nullptr};

    QSet<int> viewedIds_;
    QSet<int> favoriteIds_;
    QStringList userKeywords_;
    QList<QPair<int, QString>> paperPool_;
    QMap<int, int> userRatings_;
    double minScore_{0.3};
};
