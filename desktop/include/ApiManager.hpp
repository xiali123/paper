#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include "PaperTypes.hpp"

struct ApiResponse {
    bool success;
    QString error;
    QString message;
    QJsonObject data;
    qint64 timestamp;
};

class ApiManager : public QObject {
    Q_OBJECT

public:
    explicit ApiManager(QObject* parent = nullptr);
    ~ApiManager() = default;

    void setBaseUrl(const QString& url);
    QString baseUrl() const { return baseUrl_; }
    void setAuthToken(const QString& token);
    void clearAuthToken();

    // HTTP methods
    QNetworkReply* get(const QNetworkRequest& request);
    QNetworkReply* post(const QNetworkRequest& request, const QByteArray& data);
    QNetworkReply* put(const QNetworkRequest& request, const QByteArray& data);
    QNetworkReply* deleteResource(const QNetworkRequest& request);
    QNetworkRequest createRequest(const QString& endpoint);

    // === Core API ===
    void checkHealth();
    void searchPapers(const QString& query, const QString& year = "", const QString& level = "",
                      int offset = 0, int limit = 20);
    void advancedSearch(const QJsonObject& criteria);
    void getSearchSuggestions(const QString& prefix);
    void getSearchHistory();
    void getSavedSearches();
    void saveSearch(const QJsonObject& data);
    void getTrendingSearches();

    void getPaperDetails(int paperId);
    void createPaper(const QJsonObject& data);
    void updatePaper(int paperId, const QJsonObject& data);
    void deletePaper(int paperId);
    void getRecentPapers(int limit = 20);
    void getAllPapers(int page = 1, int limit = 20);

    // Paper interactions
    void togglePaperFavorite(int paperId, bool favorite);
    void markPaperRead(int paperId);
    void addPaperTags(int paperId, const QStringList& tags);
    void removePaperTag(int paperId, const QString& tag);

    // Stats
    void getStats(const QString& type = "overview");
    void getSystemStats();
    void getResourceStats();
    void getPerformanceStats();

    // === Crawler API ===
    void getCrawlerDashboard();
    void getCrawlerTasks(int page = 1, int limit = 20);
    void getCrawlerTaskDetails(int taskId);
    void retryCrawlerTask(int taskId);
    void getCrawlerTemplates();
    void createCrawlerTemplate(const QJsonObject& data);
    void updateCrawlerTemplate(int id, const QJsonObject& data);
    void deleteCrawlerTemplate(int id);
    void testCrawlerTemplate(int id);
    void getCrawlerSchedules();
    void createCrawlerSchedule(const QJsonObject& data);
    void triggerCrawlerSchedule(int id);
    void getCrawlerStatistics();
    void getCrawlerWorkers();

    // === AI API ===
    void aiReview(const QJsonObject& data);
    void aiChat(const QJsonObject& data);
    void aiCompare(const QJsonObject& data);
    void aiSummarize(const QJsonObject& data);
    void aiContributions(const QJsonObject& data);
    void aiKeywords(const QJsonObject& data);
    void getAiStatus();

    // AI Copilot
    void aiCopilotReview(const QJsonObject& data);
    void aiCopilotReviews(int userId);
    void generateLiteratureReview(const QJsonObject& data);
    void getLiteratureReviews();
    void generateResearchPlan(const QJsonObject& data);
    void getResearchPlans();
    void getAiCopilotRecommendations();
    void getAiCopilotStats();

    // === Recommendations ===
    void getPaperRecommendations(int page = 1, int limit = 20);
    void getSimilarPapers(int paperId);
    void getRecommendationExplanation(int paperId);
    void submitRecommendationFeedback(const QJsonObject& data);
    void getTrendingPapers();

    // === Export API ===
    void getExportFormats();
    void exportData(const QJsonObject& params);

    // === Admin API ===
    void getAdminDashboard();
    void getAdminUsers(int page = 1, int limit = 20);
    void getAdminModules();
    void toggleModule(const QString& name, bool enable);
    void getSystemMonitor();
    void getServiceMonitor();
    void getPerformanceMetrics();
    void getLoginHistory();

signals:
    // Core signals
    void healthCheckSuccess(bool healthy, const QString& message);
    void healthCheckFailed(const QString& error);
    void searchSuccess(const SearchResult& result);
    void searchFailed(const QString& error);
    void paperDetailsSuccess(const Paper& paper);
    void paperDetailsFailed(const QString& error);
    void recentPapersSuccess(const QList<Paper>& papers);
    void recentPapersFailed(const QString& error);
    void statsSuccess(const QJsonObject& stats);
    void statsFailed(const QString& error);
    void networkError(const QString& error);
    void crawlerDashboardSuccess(const QJsonObject& data);
    void crawlerTasksSuccess(const QJsonArray& tasks);
    void crawlerTemplatesSuccess(const QJsonArray& templates);
    void apiError(const QString& error);
    void aiReviewSuccess(const QJsonObject& result);
    void aiChatSuccess(const QString& response);

    // Generic JSON response signal for new APIs
    void jsonResponse(const QString& endpoint, const QJsonObject& data);
    void jsonArrayResponse(const QString& endpoint, const QJsonArray& data);
    void genericError(const QString& context, const QString& error);

private slots:
    void onHealthCheckReply();
    void onSearchReply();
    void onPaperDetailsReply();
    void onRecentPapersReply();
    void onStatsReply();
    void onCrawlerDashboardReply();
    void onCrawlerTasksReply();
    void onCrawlerTemplatesReply();
    void onAiReviewReply();
    void onAiChatReply();
    void handleNetworkError(QNetworkReply::NetworkError error);

private:
    ApiResponse parseResponse(QNetworkReply* reply);
    QString buildQueryString(const QMap<QString, QString>& params);
    void setupRequestTimeout(QNetworkReply* reply, int timeoutMs = 10000);
    bool isDuplicateRequest(const QString& key);
    void trackRequest(const QString& key);
    void untrackRequest(const QString& key);
    void retrySearch(const QString& query, const QString& year, const QString& level, int offset, int limit);

    // Fire-and-forget JSON request with generic handler
    void sendGetRequest(const QString& endpoint, const QString& context, int timeoutMs = 10000);
    void sendPostRequest(const QString& endpoint, const QJsonObject& data, const QString& context, int timeoutMs = 30000);
    void sendPutRequest(const QString& endpoint, const QJsonObject& data, const QString& context);
    void sendDeleteRequest(const QString& endpoint, const QString& context);
    void handleGenericReply(QNetworkReply* reply, const QString& context);

    QNetworkAccessManager* networkManager_;
    QString baseUrl_;
    QString authToken_;
    QSet<QString> activeRequests_;
    int searchRetryCount_{0};
    static constexpr int maxRetries_{2};

    QNetworkReply* healthReply_{nullptr};
    QNetworkReply* searchReply_{nullptr};
    QNetworkReply* paperDetailsReply_{nullptr};
    QNetworkReply* recentPapersReply_{nullptr};
    QNetworkReply* statsReply_{nullptr};
    QNetworkReply* crawlerDashboardReply_{nullptr};
    QNetworkReply* crawlerTasksReply_{nullptr};
    QNetworkReply* crawlerTemplatesReply_{nullptr};
    QNetworkReply* aiReviewReply_{nullptr};
    QNetworkReply* aiChatReply_{nullptr};
};
