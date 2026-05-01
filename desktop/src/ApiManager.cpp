#include "ApiManager.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QTimer>
#include <QDebug>

// ============================================================================
// Construction / Configuration
// ============================================================================

ApiManager::ApiManager(QObject* parent)
    : QObject(parent)
    , networkManager_(new QNetworkAccessManager(this))
{
    baseUrl_ = "http://localhost:8080";
}

void ApiManager::setBaseUrl(const QString& url) {
    baseUrl_ = url;
}

void ApiManager::setAuthToken(const QString& token) {
    authToken_ = token;
}

void ApiManager::clearAuthToken() {
    authToken_.clear();
}

// ============================================================================
// General-purpose HTTP methods
// ============================================================================

QNetworkReply* ApiManager::get(const QNetworkRequest& request) {
    return networkManager_->get(request);
}

QNetworkReply* ApiManager::post(const QNetworkRequest& request, const QByteArray& data) {
    return networkManager_->post(request, data);
}

QNetworkReply* ApiManager::put(const QNetworkRequest& request, const QByteArray& data) {
    return networkManager_->put(request, data);
}

QNetworkReply* ApiManager::deleteResource(const QNetworkRequest& request) {
    return networkManager_->deleteResource(request);
}

QNetworkRequest ApiManager::createRequest(const QString& endpoint) {
    QUrl url(baseUrl_ + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "PaperCrawlerDesktop/1.0");

    if (!authToken_.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + authToken_.toUtf8());
    }

    return request;
}

// ============================================================================
// High-level API calls
// ============================================================================

void ApiManager::checkHealth() {
    QNetworkRequest request = createRequest("/health");
    healthReply_ = networkManager_->get(request);
    setupRequestTimeout(healthReply_, 10000);
    connect(healthReply_, &QNetworkReply::finished, this, &ApiManager::onHealthCheckReply);
    connect(healthReply_, &QNetworkReply::errorOccurred, this, &ApiManager::handleNetworkError);
}

void ApiManager::searchPapers(const QString& query, const QString& year,
                              const QString& level, int offset, int limit) {
    QString dedupKey = QString("search:%1:%2:%3:%4:%5").arg(query, year, level).arg(offset).arg(limit);
    if (isDuplicateRequest(dedupKey)) return;
    trackRequest(dedupKey);
    searchRetryCount_ = 0;

    QUrl url(baseUrl_ + "/api/search");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    if (!year.isEmpty()) urlQuery.addQueryItem("year", year);
    if (!level.isEmpty()) urlQuery.addQueryItem("level", level);
    urlQuery.addQueryItem("offset", QString::number(offset));
    urlQuery.addQueryItem("limit", QString::number(limit));
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "PaperCrawlerDesktop/1.0");
    if (!authToken_.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + authToken_.toUtf8());
    }

    searchReply_ = networkManager_->get(request);
    setupRequestTimeout(searchReply_, 60000);
    connect(searchReply_, &QNetworkReply::finished, this, &ApiManager::onSearchReply);
    connect(searchReply_, &QNetworkReply::errorOccurred, this, &ApiManager::handleNetworkError);
}

void ApiManager::getPaperDetails(int paperId) {
    QNetworkRequest request = createRequest(QString("/api/papers/%1").arg(paperId));
    paperDetailsReply_ = networkManager_->get(request);
    setupRequestTimeout(paperDetailsReply_, 10000);
    connect(paperDetailsReply_, &QNetworkReply::finished, this, &ApiManager::onPaperDetailsReply);
    connect(paperDetailsReply_, &QNetworkReply::errorOccurred, this, &ApiManager::handleNetworkError);
}

void ApiManager::getRecentPapers(int limit) {
    QNetworkRequest request = createRequest(QString("/api/papers/recent?limit=%1").arg(limit));
    recentPapersReply_ = networkManager_->get(request);
    setupRequestTimeout(recentPapersReply_, 10000);
    connect(recentPapersReply_, &QNetworkReply::finished, this, &ApiManager::onRecentPapersReply);
    connect(recentPapersReply_, &QNetworkReply::errorOccurred, this, &ApiManager::handleNetworkError);
}

void ApiManager::getStats(const QString& type) {
    QNetworkRequest request = createRequest(QString("/api/stats?type=%1").arg(type));
    statsReply_ = networkManager_->get(request);
    setupRequestTimeout(statsReply_, 10000);
    connect(statsReply_, &QNetworkReply::finished, this, &ApiManager::onStatsReply);
    connect(statsReply_, &QNetworkReply::errorOccurred, this, &ApiManager::handleNetworkError);
}

// ============================================================================
// Crawler API
// ============================================================================

void ApiManager::getCrawlerDashboard() {
    QNetworkRequest request = createRequest("/api/crawler/dashboard");
    crawlerDashboardReply_ = networkManager_->get(request);
    setupRequestTimeout(crawlerDashboardReply_, 10000);
    connect(crawlerDashboardReply_, &QNetworkReply::finished, this, &ApiManager::onCrawlerDashboardReply);
}

void ApiManager::getCrawlerTasks(int page, int limit) {
    QNetworkRequest request = createRequest(
        QString("/api/crawler/tasks?page=%1&limit=%2").arg(page).arg(limit));
    crawlerTasksReply_ = networkManager_->get(request);
    setupRequestTimeout(crawlerTasksReply_, 10000);
    connect(crawlerTasksReply_, &QNetworkReply::finished, this, &ApiManager::onCrawlerTasksReply);
}

void ApiManager::getCrawlerTemplates() {
    QNetworkRequest request = createRequest("/api/crawler/templates");
    crawlerTemplatesReply_ = networkManager_->get(request);
    setupRequestTimeout(crawlerTemplatesReply_, 10000);
    connect(crawlerTemplatesReply_, &QNetworkReply::finished, this, &ApiManager::onCrawlerTemplatesReply);
}

// ============================================================================
// AI API
// ============================================================================

void ApiManager::aiReview(const QJsonObject& data) {
    QNetworkRequest request = createRequest("/api/ai/review");
    aiReviewReply_ = networkManager_->post(request, QJsonDocument(data).toJson());
    setupRequestTimeout(aiReviewReply_, 60000);
    connect(aiReviewReply_, &QNetworkReply::finished, this, &ApiManager::onAiReviewReply);
}

void ApiManager::aiChat(const QJsonObject& data) {
    QNetworkRequest request = createRequest("/api/ai/chat");
    aiChatReply_ = networkManager_->post(request, QJsonDocument(data).toJson());
    setupRequestTimeout(aiChatReply_, 30000);
    connect(aiChatReply_, &QNetworkReply::finished, this, &ApiManager::onAiChatReply);
}

// ============================================================================
// Reply handlers
// ============================================================================

void ApiManager::onHealthCheckReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    healthReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();
        QString status = json["status"].toString();
        bool isHealthy = (status == "ok" || status == "healthy");
        emit healthCheckSuccess(isHealthy, status);
    } else {
        emit healthCheckFailed("Network error: " + reply->errorString());
    }
}

void ApiManager::onSearchReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    // Untrack dedup
    QUrl url = reply->request().url();
    QString dedupKey = "search:" + url.query();
    untrackRequest(dedupKey);

    QByteArray responseData = reply->readAll();
    reply->deleteLater();
    searchReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject json = doc.object();

        SearchResult result;
        QJsonValue dataVal = json["data"];
        QJsonObject dataObj = dataVal.isObject() ? dataVal.toObject() : json;

        QJsonArray papersArray = dataObj["papers"].toArray();
        for (const QJsonValue& value : papersArray) {
            result.papers.append(Paper::fromJson(value.toObject()));
        }

        result.total = dataObj["total"].toInt(json["total"].toInt(papersArray.size()));
        result.offset = dataObj["offset"].toInt(0);
        result.limit = dataObj["limit"].toInt(papersArray.size());
        result.durationMs = dataObj.contains("duration")
            ? dataObj["duration"].toDouble()
            : dataObj["duration_ms"].toDouble(0.0);
        result.query = dataObj["keyword"].toString(dataObj["query"].toString());

        emit searchSuccess(result);
    } else {
        // Auto-retry on network failure
        if (searchRetryCount_ < maxRetries_) {
            searchRetryCount_++;
            qDebug() << "Search failed, retrying (" << searchRetryCount_ << "/" << maxRetries_ << ")";
            // Extract original params from URL
            QUrlQuery urlQuery(url.query());
            QString query = urlQuery.queryItemValue("q");
            QString year = urlQuery.queryItemValue("year");
            QString level = urlQuery.queryItemValue("level");
            int offset = urlQuery.queryItemValue("offset").toInt();
            int limit = urlQuery.queryItemValue("limit").toInt();
            QTimer::singleShot(1000 * searchRetryCount_, this, [this, query, year, level, offset, limit]() {
                retrySearch(query, year, level, offset, limit);
            });
            return;
        }
        searchRetryCount_ = 0;
        emit searchFailed("Network error: " + reply->errorString());
    }
}

void ApiManager::onPaperDetailsReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    paperDetailsReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();

        QJsonObject paperData = json.contains("paper") ? json["paper"].toObject() : json;
        Paper paper = Paper::fromJson(paperData);
        emit paperDetailsSuccess(paper);
    } else {
        emit paperDetailsFailed("Network error: " + reply->errorString());
    }
}

void ApiManager::onRecentPapersReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    recentPapersReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();

        QList<Paper> papers;
        QJsonArray papersArray = json["papers"].toArray();
        for (const QJsonValue& value : papersArray) {
            papers.append(Paper::fromJson(value.toObject()));
        }
        emit recentPapersSuccess(papers);
    } else {
        emit recentPapersFailed("Network error: " + reply->errorString());
    }
}

void ApiManager::onStatsReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    statsReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject json = doc.object();
        emit statsSuccess(json);
    } else {
        emit statsFailed("Network error: " + reply->errorString());
    }
}

// ============================================================================
// Helpers
// ============================================================================

ApiResponse ApiManager::parseResponse(QNetworkReply* reply) {
    ApiResponse response;
    response.success = false;

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject json = doc.object();

    response.success = json["success"].toBool();
    response.error = json["error"].toString();
    response.message = json["message"].toString();
    response.data = json["data"].toObject();
    response.timestamp = json["timestamp"].toVariant().toLongLong();

    return response;
}

QString ApiManager::buildQueryString(const QMap<QString, QString>& params) {
    if (params.isEmpty()) return "";

    QStringList pairs;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        QString encodedKey = QString::fromUtf8(QUrl::toPercentEncoding(it.key()));
        QString encodedValue = QString::fromUtf8(QUrl::toPercentEncoding(it.value()));
        pairs.append(encodedKey + "=" + encodedValue);
    }
    return "?" + pairs.join("&");
}

void ApiManager::handleNetworkError(QNetworkReply::NetworkError error) {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    emit networkError(reply->errorString());
}

void ApiManager::setupRequestTimeout(QNetworkReply* reply, int timeoutMs) {
    if (!reply) return;

    QTimer* timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->setInterval(timeoutMs);

    connect(timer, &QTimer::timeout, this, [reply]() {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, timer, [timer]() {
        if (timer->isActive()) timer->stop();
    });

    timer->start();
}

// ============================================================================
// Crawler / AI Reply handlers
// ============================================================================

void ApiManager::onCrawlerDashboardReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    crawlerDashboardReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        emit crawlerDashboardSuccess(doc.object());
    } else {
        emit apiError("Crawler dashboard: " + reply->errorString());
    }
}

void ApiManager::onCrawlerTasksReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    crawlerTasksReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        QJsonArray tasks = obj.contains("data") ? obj["data"].toArray() : doc.array();
        emit crawlerTasksSuccess(tasks);
    } else {
        emit apiError("Crawler tasks: " + reply->errorString());
    }
}

void ApiManager::onCrawlerTemplatesReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    crawlerTemplatesReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray templates = doc.isArray() ? doc.array() : doc.object()["data"].toArray();
        emit crawlerTemplatesSuccess(templates);
    } else {
        emit apiError("Crawler templates: " + reply->errorString());
    }
}

void ApiManager::onAiReviewReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    aiReviewReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        emit aiReviewSuccess(doc.object());
    } else {
        emit apiError("AI review: " + reply->errorString());
    }
}

void ApiManager::onAiChatReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    reply->deleteLater();
    aiChatReply_ = nullptr;

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        QString response = obj.contains("response") ? obj["response"].toString() : QString::fromUtf8(reply->readAll());
        emit aiChatSuccess(response);
    } else {
        emit apiError("AI chat: " + reply->errorString());
    }
}

// ============================================================================
// Request dedup
// ============================================================================

bool ApiManager::isDuplicateRequest(const QString& key) {
    if (activeRequests_.contains(key)) {
        qDebug() << "Duplicate request blocked:" << key;
        return true;
    }
    return false;
}

void ApiManager::trackRequest(const QString& key) {
    activeRequests_.insert(key);
}

void ApiManager::untrackRequest(const QString& key) {
    activeRequests_.remove(key);
}

void ApiManager::retrySearch(const QString& query, const QString& year,
                              const QString& level, int offset, int limit) {
    // Don't reset retry count - it accumulates across retries
    QString dedupKey = QString("search:%1:%2:%3:%4:%5").arg(query, year, level).arg(offset).arg(limit);
    trackRequest(dedupKey);

    QUrl url(baseUrl_ + "/api/search");
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    if (!year.isEmpty()) urlQuery.addQueryItem("year", year);
    if (!level.isEmpty()) urlQuery.addQueryItem("level", level);
    urlQuery.addQueryItem("offset", QString::number(offset));
    urlQuery.addQueryItem("limit", QString::number(limit));
    url.setQuery(urlQuery);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "PaperCrawlerDesktop/1.0");
    if (!authToken_.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + authToken_.toUtf8());
    }

    searchReply_ = networkManager_->get(request);
    setupRequestTimeout(searchReply_, 60000);
    connect(searchReply_, &QNetworkReply::finished, this, &ApiManager::onSearchReply);
    connect(searchReply_, &QNetworkReply::errorOccurred, this, &ApiManager::handleNetworkError);
}
