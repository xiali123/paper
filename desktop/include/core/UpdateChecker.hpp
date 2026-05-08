#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QVersionNumber>

class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);

    void checkForUpdates();
    void setCurrentVersion(const QString& version);
    QString currentVersion() const { return currentVersion_.toString(); }
    void setUpdateUrl(const QString& url);

signals:
    void updateAvailable(const QString& version, const QString& url, const QString& notes);
    void upToDate();
    void checkError(const QString& error);

private slots:
    void onCheckReply();

private:
    QNetworkAccessManager* networkManager_{nullptr};
    QVersionNumber currentVersion_;
    QString updateUrl_;
};
