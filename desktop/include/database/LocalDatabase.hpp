#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QMutex>
#include <QReadWriteLock>
#include "PaperTypes.hpp"

struct DbPaper {
    int id{-1};
    QString doi;
    QString title;
    QString authors;
    QString abstract;
    QString journal;
    QString year;
    QString volume;
    QString issue;
    QString pages;
    QString keywords;
    QString pdfUrl;
    QString source;
    QDateTime createdAt;
    QDateTime updatedAt;
    QString syncStatus;

    // Convert from unified Paper type
    static DbPaper fromPaper(const Paper& p) {
        DbPaper db;
        db.id = p.id;
        db.title = p.title;
        db.authors = p.authors;
        db.abstract = p.abstract;
        db.journal = p.journalFull.isEmpty() ? p.journal : p.journalFull;
        db.year = p.year;
        db.doi = p.doiUrl;
        return db;
    }

    Paper toPaper() const {
        Paper p;
        p.id = id;
        p.title = title;
        p.authors = authors;
        p.abstract = abstract;
        p.journal = journal;
        p.journalFull = journal;
        p.year = year;
        p.doiUrl = doi;
        return p;
    }
};

struct DbSearchResult {
    QList<Paper> papers;
    int totalCount{0};
    int localCount{0};
    int serverCount{0};
    int crawlerCount{0};
    qint64 elapsedMs{0};
};

class LocalDatabase : public QObject {
    Q_OBJECT

public:
    explicit LocalDatabase(QObject* parent = nullptr);
    ~LocalDatabase();

    bool open(const QString& dbPath = "");
    void close();
    bool isOpen() const { return db_.isOpen(); }

    int savePaper(const DbPaper& paper);
    bool updatePaper(int paperId, const DbPaper& paper);
    bool deletePaper(int paperId);
    DbPaper getPaper(int paperId);
    QList<DbPaper> getAllPapers();
    int getPaperCount();

    DbSearchResult searchPapers(const QString& keyword,
                                int offset = 0,
                                int limit = 50);

    bool existsByDoi(const QString& doi);
    bool existsByTitle(const QString& title);
    int findByTitleHash(const QString& titleHash);

    QList<DbPaper> getPendingSync();
    bool updateSyncStatus(int paperId, const QString& status);
    bool markForSync(int paperId);
    QDateTime getLastSyncTime();

    bool vacuum();
    bool backup(const QString& backupPath);

signals:
    void paperAdded(int paperId);
    void paperUpdated(int paperId);
    void paperDeleted(int paperId);
    void databaseError(const QString& error);

private:
    bool createTables();
    bool createIndexes();
    QString generateTitleHash(const QString& title);
    DbPaper fromQuery(QSqlQuery& query);

    QSqlDatabase db_;
    QReadWriteLock lock_;
    QString dbPath_;
};
