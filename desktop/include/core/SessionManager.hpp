#pragma once

#include <QObject>
#include <QJsonObject>
#include <QStringList>

class SessionManager : public QObject {
    Q_OBJECT

public:
    explicit SessionManager(QObject* parent = nullptr);

    void saveSession(const QString& keyword, int offset, int tab,
                     const QByteArray& windowGeometry, const QByteArray& windowState);
    QJsonObject restoreSession();

    void saveRecentSearch(const QString& query);
    QStringList recentSearches() const;

    void saveOpenTabs(const QList<int>& tabIds);
    QList<int> openTabs() const;

private:
    QString sessionPath() const;
    QString recentPath() const;
};
