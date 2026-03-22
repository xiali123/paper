# 测试 API 连接和数据格式

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

    // Test search API
    QUrl url("http://localhost:8080/api/search?q=test");
    QUrlQuery query;
    query.addQueryItem("limit", "3");
    url.setQuery(query);

    qDebug() << "Testing API:" << url.toString();

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = manager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            qDebug() << "\n=== Raw Response ===";
            qDebug().noquote() << data;

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "JSON Parse Error:" << parseError.errorString();
            } else {
                QJsonObject json = doc.object();
                qDebug() << "\n=== Parsed JSON ===";
                qDebug() << "Has 'papers':" << json.contains("papers");
                qDebug() << "Has 'total':" << json.contains("total");
                qDebug() << "Has 'keyword':" << json.contains("keyword");
                qDebug() << "Has 'duration':" << json.contains("duration");

                QJsonArray papers = json["papers"].toArray();
                qDebug() << "\nPapers count:" << papers.size();

                for (int i = 0; i < papers.size(); ++i) {
                    QJsonObject paper = papers[i].toObject();
                    qDebug() << "\n--- Paper" << (i+1) << "---";
                    qDebug() << "  id:" << paper["id"].toInt();
                    qDebug() << "  title:" << paper["title"].toString();
                    qDebug() << "  journal:" << paper["journal"].toString();
                    qDebug() << "  year:" << paper["year"].toString();
                    qDebug() << "  level:" << paper["level"].toString();
                    qDebug() << "  Has 'authors':" << paper.contains("authors");
                    qDebug() << "  Has 'author':" << paper.contains("author");
                    qDebug() << "  Has 'journal_full':" << paper.contains("journal_full");
                    qDebug() << "  Has 'journal_short':" << paper.contains("journal_short");
                    qDebug() << "  Has 'doi_url':" << paper.contains("doi_url");
                }
            }
        } else {
            qDebug() << "Network Error:" << reply->errorString();
        }

        app.quit();
    });

    return app.exec();
}
