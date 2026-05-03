#include "SessionManager.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>

SessionManager::SessionManager(QObject* parent)
    : QObject(parent)
{
}

QString SessionManager::sessionPath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/session.json";
}

QString SessionManager::recentPath() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/recent_searches.json";
}

void SessionManager::saveSession(const QString& keyword, int offset, int tab,
                                  const QByteArray& windowGeometry, const QByteArray& windowState) {
    QJsonObject session;
    session["keyword"] = keyword;
    session["offset"] = offset;
    session["tab"] = tab;
    session["geometry"] = QString(windowGeometry.toBase64());
    session["windowState"] = QString(windowState.toBase64());
    session["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile file(sessionPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(session).toJson(QJsonDocument::Compact));
    }
}

QJsonObject SessionManager::restoreSession() {
    QFile file(sessionPath());
    if (!file.open(QIODevice::ReadOnly)) return {};

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.object();
}

void SessionManager::saveRecentSearch(const QString& query) {
    auto recent = recentSearches();
    recent.removeAll(query);
    recent.prepend(query);
    if (recent.size() > 50) recent = recent.mid(0, 50);

    QJsonArray arr;
    for (const auto& q : recent) arr.append(q);

    QFile file(recentPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }
}

QStringList SessionManager::recentSearches() const {
    QFile file(recentPath());
    if (!file.open(QIODevice::ReadOnly)) return {};

    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
    QStringList result;
    for (const auto& v : arr) result.append(v.toString());
    return result;
}

void SessionManager::saveOpenTabs(const QList<int>& tabIds) {
    QJsonArray arr;
    for (int id : tabIds) arr.append(id);

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(dir + "/open_tabs.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }
}

QList<int> SessionManager::openTabs() const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(dir + "/open_tabs.json");
    if (!file.open(QIODevice::ReadOnly)) return {};

    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
    QList<int> result;
    for (const auto& v : arr) result.append(v.toInt());
    return result;
}
