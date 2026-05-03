#include "UpdateChecker.hpp"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , currentVersion_("1.0.0")
    , updateUrl_("https://api.github.com/repos/papercrawler/desktop/releases/latest")
{
    networkManager_ = new QNetworkAccessManager(this);
}

void UpdateChecker::setCurrentVersion(const QString& version) {
    currentVersion_ = QVersionNumber::fromString(version);
}

void UpdateChecker::setUpdateUrl(const QString& url) {
    updateUrl_ = url;
}

void UpdateChecker::checkForUpdates() {
    QNetworkRequest request(QUrl(updateUrl_));
    request.setHeader(QNetworkRequest::UserAgentHeader, "PaperCrawler/1.0");
    auto* reply = networkManager_->get(request);
    connect(reply, &QNetworkReply::finished, this, &UpdateChecker::onCheckReply);
}

void UpdateChecker::onCheckReply() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit checkError(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    auto data = doc.object();

    QString tagName = data.value("tag_name").toString();
    if (tagName.startsWith('v')) tagName = tagName.mid(1);

    QVersionNumber latest = QVersionNumber::fromString(tagName);
    if (latest.isNull()) {
        emit checkError("Invalid version format");
        return;
    }

    if (latest > currentVersion_) {
        QString htmlUrl = data.value("html_url").toString();
        QString notes = data.value("body").toString();
        emit updateAvailable(tagName, htmlUrl, notes);
    } else {
        emit upToDate();
    }
}
