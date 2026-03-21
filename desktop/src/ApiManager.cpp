#include "ApiManager.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QTimer>
#include <QDebug>
#include <QRegularExpression>

ApiPaper ApiPaper::fromJson(const QJsonObject& json) {
    ApiPaper paper;
    paper.id = json["id"].toInt();
    paper.title = json["title"].toString();

    // Handle journal field - backend returns flat string "CVPR 2024"
    QString journalStr = json["journal"].toString();
    if (!journalStr.isEmpty()) {
        // Try to parse "CVPR 2024" -> full="CVPR 2024", short="CVPR"
        QRegularExpression re("^([A-Z]+)\\s+(\\d{4})$");
        QRegularExpressionMatch match = re.match(journalStr);
        if (match.hasMatch()) {
            paper.journalFull = journalStr;
            paper.journalShort = match.captured(1);
        } else {
            paper.journalFull = journalStr;
            paper.journalShort = journalStr;
        }
    } else {
        // Fallback to old field names
        paper.journalFull = json["journal_full"].toString();
        paper.journalShort = json["journal_short"].toString();
    }

    paper.year = json["year"].toString();

    // Handle authors - backend doesn't return this field
    paper.authors = json["authors"].toString();
    if (paper.authors.isEmpty()) {
        paper.authors = json["author"].toString();
    }
    if (paper.authors.isEmpty()) {
        paper.authors = "Unknown Authors"; // Default value
    }

    paper.level = json["level"].toString();

    // Handle URLs - backend doesn't return these fields
    paper.doiUrl = json["doi_url"].toString();
    paper.journalUrl = json["journal_url"].toString();

    paper.type = json["type"].toString();
    if (paper.type.isEmpty()) {
        paper.type = json["kid"].toInt() > 0 ? "Paper" : "Unknown";
    }

    return paper;
}

ApiManager::ApiManager(QObject* parent)
    : QObject(parent), networkManager_(new QNetworkAccessManager(this)) {

    // Set default base URL
    baseUrl_ = "http://localhost:8080";
}

void ApiManager::setBaseUrl(const QString& url) {
    baseUrl_ = url;
}

void ApiManager::checkHealth() {
    QNetworkRequest request = createRequest("/health");
    healthReply_ = networkManager_->get(request);

    // setupRequestTimeout(healthReply_, 10000);  // TEMPORARILY DISABLED TO DEBUG

    connect(healthReply_, &QNetworkReply::finished,
            this, &ApiManager::onHealthCheckReply);
    connect(healthReply_, &QNetworkReply::errorOccurred,
            this, &ApiManager::handleNetworkError);
}

void ApiManager::searchPapers(const QString& query, const QString& year,
                             const QString& level, int offset, int limit) {
    // Use QUrl and QUrlQuery for proper URL construction
    QUrl url(baseUrl_ + "/api/search");

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    if (!year.isEmpty()) {
        urlQuery.addQueryItem("year", year);
    }
    if (!level.isEmpty()) {
        urlQuery.addQueryItem("level", level);
    }
    urlQuery.addQueryItem("offset", QString::number(offset));
    urlQuery.addQueryItem("limit", QString::number(limit));

    url.setQuery(urlQuery);

    qDebug() << "=== Search Request ===";
    qDebug() << "URL:" << url.toString();
    qDebug() << "Query:" << query;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "PaperCrawlerDesktop/1.0");

    searchReply_ = networkManager_->get(request);

    qDebug() << "Reply created:" << (searchReply_ != nullptr);
    qDebug() << "Reply URL:" << (searchReply_ ? searchReply_->url().toString() : "null");

    // setupRequestTimeout(searchReply_, 60000);  // TEMPORARILY DISABLED TO DEBUG

    connect(searchReply_, &QNetworkReply::finished,
            this, &ApiManager::onSearchReply);
    connect(searchReply_, &QNetworkReply::errorOccurred,
            this, &ApiManager::handleNetworkError);
}

void ApiManager::getPaperDetails(int paperId) {
    QString endpoint = QString("/api/papers/%1").arg(paperId);
    QNetworkRequest request = createRequest(endpoint);
    paperDetailsReply_ = networkManager_->get(request);

    setupRequestTimeout(paperDetailsReply_, 10000);  // 10 second timeout

    connect(paperDetailsReply_, &QNetworkReply::finished,
            this, &ApiManager::onPaperDetailsReply);
    connect(paperDetailsReply_, &QNetworkReply::errorOccurred,
            this, &ApiManager::handleNetworkError);
}

void ApiManager::getRecentPapers(int limit) {
    QString queryString = QString("?limit=%1").arg(limit);
    QNetworkRequest request = createRequest("/api/papers/recent" + queryString);
    recentPapersReply_ = networkManager_->get(request);

    setupRequestTimeout(recentPapersReply_, 10000);  // 10 second timeout

    connect(recentPapersReply_, &QNetworkReply::finished,
            this, &ApiManager::onRecentPapersReply);
    connect(recentPapersReply_, &QNetworkReply::errorOccurred,
            this, &ApiManager::handleNetworkError);
}

void ApiManager::onHealthCheckReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit healthCheckFailed("JSON解析错误: " + parseError.errorString());
            reply->deleteLater();
            healthReply_ = nullptr;
            return;
        }

        QJsonObject json = doc.object();
        QString status = json["status"].toString();

        // Backend returns "ok" instead of "healthy"
        bool isHealthy = (status == "ok" || status == "healthy");
        emit healthCheckSuccess(isHealthy, status);
    } else {
        QString errorMsg = reply->errorString();
        emit healthCheckFailed("网络错误: " + errorMsg);
    }

    reply->deleteLater();
    healthReply_ = nullptr;
}

void ApiManager::onSearchReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        qWarning() << "onSearchReply: sender() is null or not a QNetworkReply";
        return;
    }

    qDebug() << "=== Search Reply ===";
    qDebug() << "Error:" << reply->error();
    qDebug() << "Error String:" << reply->errorString();
    qDebug() << "HTTP Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit searchFailed("JSON解析错误: " + parseError.errorString());
            reply->deleteLater();
            searchReply_ = nullptr;
            return;
        }

        QJsonObject json = doc.object();
        SearchResult result;

        // Check if wrapped in {success, data} or direct format
        QJsonValue dataVal = json["data"];
        QJsonObject dataObj = dataVal.isObject() ? dataVal.toObject() : json;

        // Parse papers array
        QJsonArray papersArray = dataObj["papers"].toArray();
        for (const QJsonValue& value : papersArray) {
            result.papers.append(ApiPaper::fromJson(value.toObject()));
        }

        // Get metadata - try multiple field names
        result.total = dataObj["total"].toInt(
                        json["total"].toInt(
                        papersArray.size()));

        result.offset = dataObj["offset"].toInt(0);
        result.limit = dataObj["limit"].toInt(papersArray.size());

        // Try "duration" (frontend format) or "duration_ms" (backend format)
        if (dataObj.contains("duration")) {
            result.durationMs = dataObj["duration"].toDouble();
        } else {
            result.durationMs = dataObj["duration_ms"].toDouble(0.0);
        }

        // Try "keyword" or "query"
        result.query = dataObj["keyword"].toString();
        if (result.query.isEmpty()) {
            result.query = dataObj["query"].toString();
        }

        emit searchSuccess(result);
    } else {
        QString errorMsg = reply->errorString();
        emit searchFailed("网络错误: " + errorMsg);
    }

    reply->deleteLater();
    searchReply_ = nullptr;
}

void ApiManager::onPaperDetailsReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit paperDetailsFailed("JSON解析错误: " + parseError.errorString());
            reply->deleteLater();
            paperDetailsReply_ = nullptr;
            return;
        }

        QJsonObject json = doc.object();

        // Backend returns paper directly or wrapped in "paper" field
        QJsonObject paperData = json.contains("paper") ? json["paper"].toObject() : json;

        ApiPaper paper = parsePaper(paperData);
        emit paperDetailsSuccess(paper);
    } else {
        QString errorMsg = reply->errorString();
        emit paperDetailsFailed("网络错误: " + errorMsg);
    }

    reply->deleteLater();
    paperDetailsReply_ = nullptr;
}

void ApiManager::onRecentPapersReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit recentPapersFailed("JSON解析错误: " + parseError.errorString());
            reply->deleteLater();
            recentPapersReply_ = nullptr;
            return;
        }

        QJsonObject json = doc.object();
        QList<ApiPaper> papers;

        // Backend returns papers array at root level
        QJsonArray papersArray = json["papers"].toArray();
        for (const QJsonValue& value : papersArray) {
            papers.append(ApiPaper::fromJson(value.toObject()));
        }

        emit recentPapersSuccess(papers);
    } else {
        QString errorMsg = reply->errorString();
        emit recentPapersFailed("网络错误: " + errorMsg);
    }

    reply->deleteLater();
    recentPapersReply_ = nullptr;
}

ApiResponse ApiManager::parseResponse(QNetworkReply* reply) {
    ApiResponse response;
    response.success = false;

    QByteArray data = reply->readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        response.error = "JSON_PARSE_ERROR";
        response.message = "Failed to parse response: " + parseError.errorString();
        return response;
    }

    QJsonObject json = doc.object();
    response.success = json["success"].toBool();
    response.error = json["error"].toString();
    response.message = json["message"].toString();
    response.data = json["data"].toObject();
    response.timestamp = json["timestamp"].toVariant().toLongLong();

    return response;
}

ApiPaper ApiManager::parsePaper(const QJsonObject& json) {
    return ApiPaper::fromJson(json);
}

QNetworkRequest ApiManager::createRequest(const QString& endpoint) {
    // Parse endpoint into path and query string
    QString path = endpoint;
    QString queryString;

    int queryPos = endpoint.indexOf('?');
    if (queryPos > 0) {
        path = endpoint.left(queryPos);
        queryString = endpoint.mid(queryPos);
    }

    // Use QUrl for proper URL construction
    QUrl url(baseUrl_ + path);

    // Add query parameters using QUrlQuery (better than manual encoding)
    if (!queryString.isEmpty() || !endpoint.contains('?')) {
        // For cases where buildQueryString was already called, the query string is already attached
        // But we're rebuilding it properly here
    }

    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "PaperCrawlerDesktop/1.0");

    return request;
}

QString ApiManager::buildQueryString(const QMap<QString, QString>& params) {
    if (params.isEmpty()) return "";

    QStringList pairs;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        // Properly encode both key and value
        QString encodedKey = QString::fromUtf8(QUrl::toPercentEncoding(it.key()));
        QString encodedValue = QString::fromUtf8(QUrl::toPercentEncoding(it.value()));
        pairs.append(encodedKey + "=" + encodedValue);
    }

    return "?" + pairs.join("&");
}

void ApiManager::handleNetworkError(QNetworkReply::NetworkError error) {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        qWarning() << "handleNetworkError: sender() is null or not a QNetworkReply";
        return;
    }

    QString errorMsg = reply->errorString();
    qDebug() << "=== Network Error ===";
    qDebug() << "Error code:" << error;
    qDebug() << "Error string:" << errorMsg;
    qDebug() << "URL:" << reply->url().toString();

    emit networkError(errorMsg);
}

void ApiManager::setupRequestTimeout(QNetworkReply* reply, int timeoutMs) {
    if (!reply) {
        qWarning() << "setupRequestTimeout: reply is null";
        return;
    }

    qDebug() << "Setting up timeout:" << timeoutMs << "ms for" << reply->url().toString();

    QTimer* timer = new QTimer(reply);  // Parent to reply, so it gets deleted with reply
    timer->setSingleShot(true);
    timer->setInterval(timeoutMs);

    connect(timer, &QTimer::timeout, this, [this, reply, timer]() {
        if (reply && reply->isRunning()) {
            qWarning() << "Request timeout, aborting:" << reply->url();
            reply->abort();
        }
    });

    // Stop timer when reply finishes (both success and error)
    connect(reply, &QNetworkReply::finished, timer, [timer]() {
        qDebug() << "Request finished, stopping timer";
        if (timer->isActive()) {
            timer->stop();
        }
    });

    timer->start();
}
