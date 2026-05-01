#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include "PaperTypes.hpp"

// API response structure
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

    // Auth header injection
    void setAuthToken(const QString& token);
    void clearAuthToken();

    // HTTP methods - general purpose
    QNetworkReply* get(const QNetworkRequest& request);
    QNetworkReply* post(const QNetworkRequest& request, const QByteArray& data);
    QNetworkReply* put(const QNetworkRequest& request, const QByteArray& data);
    QNetworkReply* deleteResource(const QNetworkRequest& request);

    // Convenience: build authenticated request
    QNetworkRequest createRequest(const QString& endpoint);

    // High-level API calls
    void checkHealth();
    void searchPapers(const QString& query, const QString& year = "", const QString& level = "",
                      int offset = 0, int limit = 20);
    void getPaperDetails(int paperId);
    void getRecentPapers(int limit = 20);
    void getStats(const QString& type = "overview");

signals:
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

private slots:
    void onHealthCheckReply();
    void onSearchReply();
    void onPaperDetailsReply();
    void onRecentPapersReply();
    void onStatsReply();
    void handleNetworkError(QNetworkReply::NetworkError error);

private:
    ApiResponse parseResponse(QNetworkReply* reply);
    QString buildQueryString(const QMap<QString, QString>& params);
    void setupRequestTimeout(QNetworkReply* reply, int timeoutMs = 10000);

    QNetworkAccessManager* networkManager_;
    QString baseUrl_;
    QString authToken_;

    QNetworkReply* healthReply_{nullptr};
    QNetworkReply* searchReply_{nullptr};
    QNetworkReply* paperDetailsReply_{nullptr};
    QNetworkReply* recentPapersReply_{nullptr};
    QNetworkReply* statsReply_{nullptr};
};
