#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QMutex>
#include <QReadWriteLock>

/**
 * @brief 数据库论文结构
 */
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
};

/**
 * @brief 数据库搜索结果
 */
struct DbSearchResult {
    QList<DbPaper> papers;
    int totalCount{0};
    int localCount{0};
    int serverCount{0};
    int crawlerCount{0};
    qint64 elapsedMs{0};
};

/**
 * @brief 本地数据库管理器
 *
 * 功能：
 * - SQLite 数据库管理
 * - 论文 CRUD 操作
 * - 全文搜索
 * - 去重检测
 * - 同步状态管理
 */
class LocalDatabase : public QObject {
    Q_OBJECT

public:
    explicit LocalDatabase(QObject* parent = nullptr);
    ~LocalDatabase();

    // 初始化
    bool open(const QString& dbPath = "");
    void close();
    bool isOpen() const { return db_.isOpen(); }

    // 论文操作
    int savePaper(const DbPaper& paper);
    bool updatePaper(int paperId, const DbPaper& paper);
    bool deletePaper(int paperId);
    DbPaper getPaper(int paperId);
    QList<DbPaper> getAllPapers();
    int getPaperCount();

    // 搜索
    DbSearchResult searchPapers(const QString& keyword,
                                int offset = 0,
                                int limit = 50);

    // 去重
    bool existsByDoi(const QString& doi);
    bool existsByTitle(const QString& title);
    int findByTitleHash(const QString& titleHash);

    // 同步管理
    QList<DbPaper> getPendingSync();
    bool updateSyncStatus(int paperId, const QString& status);
    bool markForSync(int paperId);
    QDateTime getLastSyncTime();

    // 数据库维护
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
    QReadWriteLock lock_;  // 线程安全
    QString dbPath_;
};
