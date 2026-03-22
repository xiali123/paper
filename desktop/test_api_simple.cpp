#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QNetworkAccessManager manager;

    QUrl url("http://localhost:8080/api/search");
    QUrlQuery query;
    query.addQueryItem("q", "test");
    query.addQueryItem("limit", "3");
    url.setQuery(query);

    qDebug() << "Testing API:" << url.toString();

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = manager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            qDebug() << "Raw response:" << data.left(500);

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON Parse Error:" << parseError.errorString();
            } else {
                QJsonObject json = doc.object();
                QJsonArray papers = json["papers"].toArray();
                qDebug() << "Papers count:" << papers.size();

                for (int i = 0; i < std::min(3, papers.size()); ++i) {
                    QJsonObject paper = papers[i].toObject();
                    qDebug() << "Paper" << (i+1) << ":";
                    qDebug() << "  title:" << paper["title"].toString();
                    qDebug() << "  journal:" << paper["journal"].toString();
                }
            }
        } else {
            qDebug() << "Network Error:" << reply->errorString();
        }

        app.quit();
    });

    return app.exec();
}
