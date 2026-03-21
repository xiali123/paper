#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief API response structure
 */
struct ApiResponse {
    bool success;
    QString error;
    QString message;
    QJsonObject data;
    qint64 timestamp;
};

/**
 * @brief Paper structure matching backend API
 */
struct ApiPaper {
    int id;
    QString title;
    QString journalFull;
    QString journalShort;
    QString year;
    QString authors;
    QString level;
    QString doiUrl;
    QString journalUrl;
    QString type;

    // Conversion from JSON
    static ApiPaper fromJson(const QJsonObject& json);
};

/**
 * @brief Search result structure
 */
struct SearchResult {
    QList<ApiPaper> papers;
    int total;
    int offset;
    int limit;
    qreal durationMs;
    QString query;
};

/**
 * @brief API manager for PaperCrawler backend
 *
 * Handles HTTP requests to the backend API:
 * - Health check
 * - Search papers
 * - Get paper details
 * - Get recent papers
 */
class ApiManager : public QObject {
    Q_OBJECT

public:
    explicit ApiManager(QObject* parent = nullptr);
    ~ApiManager() = default;

    // Configuration
    void setBaseUrl(const QString& url);
    QString baseUrl() const { return baseUrl_; }

    // API endpoints
    void checkHealth();
    void searchPapers(const QString& query, const QString& year = "", const QString& level = "",
                      int offset = 0, int limit = 20);
    void getPaperDetails(int paperId);
    void getRecentPapers(int limit = 20);

signals:
    // Health check signals
    void healthCheckSuccess(bool healthy, const QString& message);
    void healthCheckFailed(const QString& error);

    // Search signals
    void searchSuccess(const SearchResult& result);
    void searchFailed(const QString& error);

    // Paper details signals
    void paperDetailsSuccess(const ApiPaper& paper);
    void paperDetailsFailed(const QString& error);

    // Recent papers signals
    void recentPapersSuccess(const QList<ApiPaper>& papers);
    void recentPapersFailed(const QString& error);

    // Network errors
    void networkError(const QString& error);

private slots:
    void onHealthCheckReply();
    void onSearchReply();
    void onPaperDetailsReply();
    void onRecentPapersReply();

private slots:
    void handleNetworkError(QNetworkReply::NetworkError error);

private:
    // Helper methods
    ApiResponse parseResponse(QNetworkReply* reply);
    ApiPaper parsePaper(const QJsonObject& json);
    QNetworkRequest createRequest(const QString& endpoint);
    QString buildQueryString(const QMap<QString, QString>& params);
    void setupRequestTimeout(QNetworkReply* reply, int timeoutMs = 10000);

    // Network manager
    QNetworkAccessManager* networkManager_;

    // Configuration
    QString baseUrl_;

    // Store reply pointers
    QNetworkReply* healthReply_{nullptr};
    QNetworkReply* searchReply_{nullptr};
    QNetworkReply* paperDetailsReply_{nullptr};
    QNetworkReply* recentPapersReply_{nullptr};
};
