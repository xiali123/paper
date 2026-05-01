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
