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
// Generic fire-and-forget request methods
// ============================================================================

void ApiManager::sendGetRequest(const QString& endpoint, const QString& context, int timeoutMs) {
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = networkManager_->get(request);
    setupRequestTimeout(reply, timeoutMs);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
        handleGenericReply(reply, context);
    });
}

void ApiManager::sendPostRequest(const QString& endpoint, const QJsonObject& data,
                                  const QString& context, int timeoutMs) {
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = networkManager_->post(request, QJsonDocument(data).toJson());
    setupRequestTimeout(reply, timeoutMs);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
        handleGenericReply(reply, context);
    });
}

void ApiManager::sendPutRequest(const QString& endpoint, const QJsonObject& data,
                                 const QString& context) {
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = networkManager_->put(request, QJsonDocument(data).toJson());
    setupRequestTimeout(reply, 10000);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
        handleGenericReply(reply, context);
    });
}

void ApiManager::sendDeleteRequest(const QString& endpoint, const QString& context) {
    QNetworkRequest request = createRequest(endpoint);
    QNetworkReply* reply = networkManager_->deleteResource(request);
    setupRequestTimeout(reply, 10000);
    connect(reply, &QNetworkReply::finished, this, [this, reply, context]() {
        handleGenericReply(reply, context);
    });
}

void ApiManager::handleGenericReply(QNetworkReply* reply, const QString& context) {
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            // Unwrap if wrapped in data/response field
            if (obj.contains("data") && obj["data"].isObject()) {
                emit jsonResponse(context, obj["data"].toObject());
            } else {
                emit jsonResponse(context, obj);
            }
        } else if (doc.isArray()) {
            emit jsonArrayResponse(context, doc.array());
        } else {
            QJsonObject wrapper;
            wrapper["raw"] = QString::fromUtf8(data);
            emit jsonResponse(context, wrapper);
        }
    } else {
        emit genericError(context, reply->errorString());
    }
}

// ============================================================================
// Advanced Search
// ============================================================================

void ApiManager::advancedSearch(const QJsonObject& criteria) {
    sendPostRequest("/api/search/advanced", criteria, "search/advanced", 60000);
}

void ApiManager::getSearchSuggestions(const QString& prefix) {
    sendGetRequest(QString("/api/search/suggest?q=%1").arg(prefix), "search/suggest");
}

void ApiManager::getSearchHistory() {
    sendGetRequest("/api/search/history", "search/history");
}

void ApiManager::getSavedSearches() {
    sendGetRequest("/api/search/saved", "search/saved");
}

void ApiManager::saveSearch(const QJsonObject& data) {
    sendPostRequest("/api/search/save", data, "search/save");
}

void ApiManager::getTrendingSearches() {
    sendGetRequest("/api/search/trending", "search/trending");
}

// ============================================================================
// Paper CRUD
// ============================================================================

void ApiManager::createPaper(const QJsonObject& data) {
    sendPostRequest("/api/papers", data, "papers/create");
}

void ApiManager::updatePaper(int paperId, const QJsonObject& data) {
    sendPutRequest(QString("/api/papers/%1").arg(paperId), data, "papers/update");
}

void ApiManager::deletePaper(int paperId) {
    sendDeleteRequest(QString("/api/papers/%1").arg(paperId), "papers/delete");
}

void ApiManager::getAllPapers(int page, int limit) {
    sendGetRequest(QString("/api/papers?page=%1&limit=%2").arg(page).arg(limit), "papers/list");
}

void ApiManager::togglePaperFavorite(int paperId, bool favorite) {
    QJsonObject data;
    data["favorite"] = favorite;
    sendPostRequest(QString("/api/papers/%1/favorite").arg(paperId), data, "papers/favorite");
}

void ApiManager::markPaperRead(int paperId) {
    sendPostRequest(QString("/api/papers/%1/read").arg(paperId), QJsonObject(), "papers/read");
}

void ApiManager::addPaperTags(int paperId, const QStringList& tags) {
    QJsonObject data;
    QJsonArray arr;
    for (const auto& t : tags) arr.append(t);
    data["tags"] = arr;
    sendPostRequest(QString("/api/papers/%1/tags").arg(paperId), data, "papers/tags");
}

void ApiManager::removePaperTag(int paperId, const QString& tag) {
    sendDeleteRequest(QString("/api/papers/%1/tags/%2").arg(paperId).arg(tag), "papers/tag-remove");
}

// ============================================================================
// Detailed Stats
// ============================================================================

void ApiManager::getSystemStats() {
    sendGetRequest("/api/stats/system", "stats/system");
}

void ApiManager::getResourceStats() {
    sendGetRequest("/api/stats/resources", "stats/resources");
}

void ApiManager::getPerformanceStats() {
    sendGetRequest("/api/stats/performance", "stats/performance");
}

// ============================================================================
// Crawler Advanced
// ============================================================================

void ApiManager::getCrawlerTaskDetails(int taskId) {
    sendGetRequest(QString("/api/crawler/tasks/%1").arg(taskId), "crawler/task-details");
}

void ApiManager::retryCrawlerTask(int taskId) {
    sendPostRequest(QString("/api/crawler/tasks/%1/retry").arg(taskId), QJsonObject(), "crawler/retry");
}

void ApiManager::createCrawlerTemplate(const QJsonObject& data) {
    sendPostRequest("/api/crawler/templates", data, "crawler/template-create");
}

void ApiManager::updateCrawlerTemplate(int id, const QJsonObject& data) {
    sendPutRequest(QString("/api/crawler/templates/%1").arg(id), data, "crawler/template-update");
}

void ApiManager::deleteCrawlerTemplate(int id) {
    sendDeleteRequest(QString("/api/crawler/templates/%1").arg(id), "crawler/template-delete");
}

void ApiManager::testCrawlerTemplate(int id) {
    sendPostRequest(QString("/api/crawler/templates/%1/test").arg(id), QJsonObject(), "crawler/template-test", 30000);
}

void ApiManager::getCrawlerSchedules() {
    sendGetRequest("/api/crawler/schedules", "crawler/schedules");
}

void ApiManager::createCrawlerSchedule(const QJsonObject& data) {
    sendPostRequest("/api/crawler/schedules", data, "crawler/schedule-create");
}

void ApiManager::triggerCrawlerSchedule(int id) {
    sendPostRequest(QString("/api/crawler/schedules/%1/trigger").arg(id), QJsonObject(), "crawler/schedule-trigger");
}

void ApiManager::getCrawlerStatistics() {
    sendGetRequest("/api/crawler/statistics", "crawler/statistics");
}

void ApiManager::getCrawlerWorkers() {
    sendGetRequest("/api/crawler/workers", "crawler/workers");
}

// ============================================================================
// AI Advanced
// ============================================================================

void ApiManager::aiCompare(const QJsonObject& data) {
    sendPostRequest("/api/ai/compare", data, "ai/compare", 60000);
}

void ApiManager::aiSummarize(const QJsonObject& data) {
    sendPostRequest("/api/ai/summarize", data, "ai/summarize", 60000);
}

void ApiManager::aiContributions(const QJsonObject& data) {
    sendPostRequest("/api/ai/contributions", data, "ai/contributions", 60000);
}

void ApiManager::aiKeywords(const QJsonObject& data) {
    sendPostRequest("/api/ai/keywords", data, "ai/keywords");
}

void ApiManager::getAiStatus() {
    sendGetRequest("/api/ai/status", "ai/status");
}

// ============================================================================
// AI Copilot
// ============================================================================

void ApiManager::aiCopilotReview(const QJsonObject& data) {
    sendPostRequest("/api/ai/copilot/review", data, "ai/copilot/review", 60000);
}

void ApiManager::aiCopilotReviews(int userId) {
    sendGetRequest(QString("/api/ai/copilot/reviews/%1").arg(userId), "ai/copilot/reviews");
}

void ApiManager::generateLiteratureReview(const QJsonObject& data) {
    sendPostRequest("/api/ai/copilot/literature-review/generate", data, "ai/copilot/literature-review", 60000);
}

void ApiManager::getLiteratureReviews() {
    sendGetRequest("/api/ai/copilot/literature-reviews", "ai/copilot/literature-reviews");
}

void ApiManager::generateResearchPlan(const QJsonObject& data) {
    sendPostRequest("/api/ai/copilot/research-plan/generate", data, "ai/copilot/research-plan", 60000);
}

void ApiManager::getResearchPlans() {
    sendGetRequest("/api/ai/copilot/research-plans", "ai/copilot/research-plans");
}

void ApiManager::getAiCopilotRecommendations() {
    sendGetRequest("/api/ai/copilot/recommendations", "ai/copilot/recommendations");
}

void ApiManager::getAiCopilotStats() {
    sendGetRequest("/api/ai/copilot/stats", "ai/copilot/stats");
}

// ============================================================================
// Recommendations
// ============================================================================

void ApiManager::getPaperRecommendations(int page, int limit) {
    sendGetRequest(QString("/api/recommendations/papers?page=%1&limit=%2").arg(page).arg(limit), "recommendations/papers");
}

void ApiManager::getSimilarPapers(int paperId) {
    sendGetRequest(QString("/api/recommendations/similar/%1").arg(paperId), "recommendations/similar");
}

void ApiManager::getRecommendationExplanation(int paperId) {
    sendGetRequest(QString("/api/recommendations/explain/%1").arg(paperId), "recommendations/explain");
}

void ApiManager::submitRecommendationFeedback(const QJsonObject& data) {
    sendPostRequest("/api/recommendations/feedback", data, "recommendations/feedback");
}

void ApiManager::getTrendingPapers() {
    sendGetRequest("/api/recommendations/trending", "recommendations/trending");
}

// ============================================================================
// Export API
// ============================================================================

void ApiManager::getExportFormats() {
    sendGetRequest("/api/export/formats", "export/formats");
}

void ApiManager::exportData(const QJsonObject& params) {
    sendPostRequest("/api/export", params, "export/data", 60000);
}

// ============================================================================
// Admin API
// ============================================================================

void ApiManager::getAdminDashboard() {
    sendGetRequest("/api/admin/dashboard", "admin/dashboard");
}

void ApiManager::getAdminUsers(int page, int limit) {
    sendGetRequest(QString("/api/admin/users?page=%1&limit=%2").arg(page).arg(limit), "admin/users");
}

void ApiManager::getAdminModules() {
    sendGetRequest("/api/admin/modules", "admin/modules");
}

void ApiManager::toggleModule(const QString& name, bool enable) {
    QString endpoint = enable
        ? QString("/api/admin/modules/%1/enable").arg(name)
        : QString("/api/admin/modules/%1/disable").arg(name);
    sendPostRequest(endpoint, QJsonObject(), "admin/module-toggle");
}

void ApiManager::getSystemMonitor() {
    sendGetRequest("/api/admin/monitor/system", "admin/monitor/system");
}

void ApiManager::getServiceMonitor() {
    sendGetRequest("/api/admin/monitor/services", "admin/monitor/services");
}

void ApiManager::getPerformanceMetrics() {
    sendGetRequest("/api/admin/performance/metrics", "admin/performance");
}

void ApiManager::getLoginHistory() {
    sendGetRequest("/api/admin/security/login-history", "admin/login-history");
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
