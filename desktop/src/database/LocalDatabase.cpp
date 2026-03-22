#include "database/LocalDatabase.hpp"
#include <QSqlError>
#include <QSqlRecord>
#include <QCryptographicHash>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QElapsedTimer>

LocalDatabase::LocalDatabase(QObject* parent)
    : QObject(parent) {

    // 数据库连接名称
    db_ = QSqlDatabase::addDatabase("QSQLITE", "LocalPaperConnection");
}

LocalDatabase::~LocalDatabase() {
    close();
}

bool LocalDatabase::open(const QString& dbPath) {
    QWriteLocker locker(&lock_);

    if (db_.isOpen()) {
        close();
    }

    // 使用默认路径或指定路径
    if (dbPath.isEmpty()) {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataPath);
        dbPath_ = appDataPath + "/papers.db";
    } else {
        dbPath_ = dbPath;
    }

    db_.setDatabaseName(dbPath_);

    if (!db_.open()) {
        QString error = db_.lastError().text();
        qCritical() << "Failed to open database:" << error;
        emit databaseError(error);
        return false;
    }

    qDebug() << "Database opened:" << dbPath_;

    // 创建表和索引
    if (!createTables() || !createIndexes()) {
        return false;
    }

    return true;
}

void LocalDatabase::close() {
    QWriteLocker locker(&lock_);
    if (db_.isOpen()) {
        db_.close();
        qDebug() << "Database closed";
    }
}

bool LocalDatabase::createTables() {
    QSqlQuery query(db_);

    // 论文表
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS papers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            doi TEXT UNIQUE,
            title TEXT NOT NULL,
            title_hash TEXT UNIQUE,
            authors TEXT,
            abstract TEXT,
            journal TEXT,
            year TEXT,
            volume TEXT,
            issue TEXT,
            pages TEXT,
            keywords TEXT,
            pdf_url TEXT,
            source TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            sync_status TEXT DEFAULT 'pending'
        )
    )";

    if (!query.exec(sql)) {
        qCritical() << "Failed to create papers table:" << query.lastError().text();
        emit databaseError(query.lastError().text());
        return false;
    }

    qDebug() << "Tables created successfully";
    return true;
}

bool LocalDatabase::createIndexes() {
    QSqlQuery query(db_);

    // title_hash 索引（快速去重）
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_title_hash ON papers(title_hash)")) {
        qWarning() << "Failed to create title_hash index:" << query.lastError().text();
    }

    // doi 索引
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_doi ON papers(doi)")) {
        qWarning() << "Failed to create doi index:" << query.lastError().text();
    }

    // 全文搜索索引（SQLite FTS5）
    QString ftsSql = R"(
        CREATE VIRTUAL TABLE IF NOT EXISTS papers_fts USING fts5(
            title,
            authors,
            abstract,
            keywords,
            content='papers',
            content_rowid='id'
        )
    )";

    if (query.exec(ftsSql)) {
        // 触发器：自动同步到 FTS 表
        QString triggerSql = R"(
            CREATE TRIGGER IF NOT EXISTS papers_ai AFTER INSERT ON papers BEGIN
                INSERT INTO papers_fts(rowid, title, authors, abstract, keywords)
                VALUES (new.id, new.title, new.authors, new.abstract, new.keywords);
            END;

            CREATE TRIGGER IF NOT EXISTS papers_ad AFTER DELETE ON papers BEGIN
                INSERT INTO papers_fts(papers_fts, rowid, title, authors, abstract, keywords)
                VALUES ('delete', old.id, old.title, old.authors, old.abstract, old.keywords);
            END;

            CREATE TRIGGER IF NOT EXISTS papers_au AFTER UPDATE ON papers BEGIN
                INSERT INTO papers_fts(papers_fts, rowid, title, authors, abstract, keywords)
                VALUES ('delete', old.id, old.title, old.authors, old.abstract, new.keywords);
                INSERT INTO papers_fts(rowid, title, authors, abstract, keywords)
                VALUES (new.id, new.title, new.authors, new.abstract, new.keywords);
            END;
        )";
        query.exec(triggerSql);
    } else {
        qWarning() << "Failed to create FTS table:" << query.lastError().text();
    }

    qDebug() << "Indexes created successfully";
    return true;
}

int LocalDatabase::savePaper(const DbPaper& paper) {
    QWriteLocker locker(&lock_);

    QSqlQuery query(db_);
    query.prepare(R"(
        INSERT INTO papers (
            doi, title, title_hash, authors, abstract, journal, year,
            volume, issue, pages, keywords, pdf_url, source, sync_status
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");

    QString titleHash = generateTitleHash(paper.title);

    query.addBindValue(paper.doi);
    query.addBindValue(paper.title);
    query.addBindValue(titleHash);
    query.addBindValue(paper.authors);
    query.addBindValue(paper.abstract);
    query.addBindValue(paper.journal);
    query.addBindValue(paper.year);
    query.addBindValue(paper.volume);
    query.addBindValue(paper.issue);
    query.addBindValue(paper.pages);
    query.addBindValue(paper.keywords);
    query.addBindValue(paper.pdfUrl);
    query.addBindValue(paper.source);
    query.addBindValue("pending");

    if (!query.exec()) {
        QString error = query.lastError().text();
        if (error.contains("UNIQUE constraint failed")) {
            // DOI 或标题已存在
            qDebug() << "Paper already exists (duplicate)";
            return -1;
        }
        qCritical() << "Failed to save paper:" << error;
        emit databaseError(error);
        return -1;
    }

    int paperId = query.lastInsertId().toInt();
    qDebug() << "Paper saved with ID:" << paperId;
    emit paperAdded(paperId);

    return paperId;
}

bool LocalDatabase::updatePaper(int paperId, const DbPaper& paper) {
    QWriteLocker locker(&lock_);

    QSqlQuery query(db_);
    query.prepare(R"(
        UPDATE papers SET
            doi = ?,
            title = ?,
            title_hash = ?,
            authors = ?,
            abstract = ?,
            journal = ?,
            year = ?,
            volume = ?,
            issue = ?,
            pages = ?,
            keywords = ?,
            pdf_url = ?,
            source = ?,
            updated_at = CURRENT_TIMESTAMP,
            sync_status = 'pending'
        WHERE id = ?
    )");

    QString titleHash = generateTitleHash(paper.title);

    query.addBindValue(paper.doi);
    query.addBindValue(paper.title);
    query.addBindValue(titleHash);
    query.addBindValue(paper.authors);
    query.addBindValue(paper.abstract);
    query.addBindValue(paper.journal);
    query.addBindValue(paper.year);
    query.addBindValue(paper.volume);
    query.addBindValue(paper.issue);
    query.addBindValue(paper.pages);
    query.addBindValue(paper.keywords);
    query.addBindValue(paper.pdfUrl);
    query.addBindValue(paper.source);
    query.addBindValue(paperId);

    if (!query.exec()) {
        QString error = query.lastError().text();
        qCritical() << "Failed to update paper:" << error;
        emit databaseError(error);
        return false;
    }

    qDebug() << "Paper updated:" << paperId;
    emit paperUpdated(paperId);
    return true;
}

bool LocalDatabase::deletePaper(int paperId) {
    QWriteLocker locker(&lock_);

    QSqlQuery query(db_);
    query.prepare("DELETE FROM papers WHERE id = ?");
    query.addBindValue(paperId);

    if (!query.exec()) {
        QString error = query.lastError().text();
        qCritical() << "Failed to delete paper:" << error;
        emit databaseError(error);
        return false;
    }

    qDebug() << "Paper deleted:" << paperId;
    emit paperDeleted(paperId);
    return true;
}

DbPaper LocalDatabase::getPaper(int paperId) {
    QReadLocker locker(&lock_);

    QSqlQuery query(db_);
    query.prepare("SELECT * FROM papers WHERE id = ?");
    query.addBindValue(paperId);

    if (!query.exec() || !query.next()) {
        qWarning() << "Paper not found:" << paperId;
        return DbPaper();
    }

    return fromQuery(query);
}

QList<DbPaper> LocalDatabase::getAllPapers() {
    QReadLocker locker(&lock_);

    QList<DbPaper> papers;
    QSqlQuery query("SELECT * FROM papers ORDER BY created_at DESC", db_);

    if (!query.exec()) {
        qCritical() << "Failed to get all papers:" << query.lastError().text();
        return papers;
    }

    while (query.next()) {
        papers.append(fromQuery(query));
    }

    return papers;
}

int LocalDatabase::getPaperCount() {
    QReadLocker locker(&lock_);

    QSqlQuery query("SELECT COUNT(*) FROM papers", db_);
    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

DbSearchResult LocalDatabase::searchPapers(const QString& keyword, int offset, int limit) {
    QReadLocker locker(&lock_);

    DbSearchResult result;
    QElapsedTimer timer;
    timer.start();

    QSqlQuery query(db_);

    // 使用 FTS5 全文搜索
    QString sql = R"(
        SELECT p.* FROM papers p
        INNER JOIN papers_fts fts ON p.id = fts.rowid
        WHERE papers_fts MATCH ?
        ORDER BY p.created_at DESC
        LIMIT ? OFFSET ?
    )";

    query.prepare(sql);
    query.addBindValue(keyword + "*");  // "*" 表示前缀匹配
    query.addBindValue(limit);
    query.addBindValue(offset);

    if (!query.exec()) {
        qCritical() << "Search failed:" << query.lastError().text();
        return result;
    }

    while (query.next()) {
        result.papers.append(fromQuery(query));
    }

    // 获取总数
    QSqlQuery countQuery(db_);
    countQuery.prepare(R"(
        SELECT COUNT(*) FROM papers p
        INNER JOIN papers_fts fts ON p.id = fts.rowid
        WHERE papers_fts MATCH ?
    )");
    countQuery.addBindValue(keyword + "*");

    if (countQuery.exec() && countQuery.next()) {
        result.totalCount = countQuery.value(0).toInt();
    } else {
        result.totalCount = result.papers.size();
    }

    result.localCount = result.papers.size();
    result.elapsedMs = timer.elapsed();

    qDebug() << "Search completed:" << result.papers.size()
             << "papers in" << result.elapsedMs << "ms";

    return result;
}

bool LocalDatabase::existsByDoi(const QString& doi) {
    if (doi.isEmpty()) return false;

    QReadLocker locker(&lock_);

    QSqlQuery query(db_);
    query.prepare("SELECT COUNT(*) FROM papers WHERE doi = ?");
    query.addBindValue(doi);

    if (!query.exec() || !query.next()) {
        return false;
    }

    return query.value(0).toInt() > 0;
}

bool LocalDatabase::existsByTitle(const QString& title) {
    QString titleHash = generateTitleHash(title);
    return findByTitleHash(titleHash) > 0;
}

int LocalDatabase::findByTitleHash(const QString& titleHash) {
    QReadLocker locker(&lock_);

    QSqlQuery query(db_);
    query.prepare("SELECT id FROM papers WHERE title_hash = ?");
    query.addBindValue(titleHash);

    if (!query.exec() || !query.next()) {
        return -1;
    }

    return query.value(0).toInt();
}

QList<DbPaper> LocalDatabase::getPendingSync() {
    QReadLocker locker(&lock_);

    QList<DbPaper> papers;
    QSqlQuery query("SELECT * FROM papers WHERE sync_status = 'pending'", db_);

    if (!query.exec()) {
        qCritical() << "Failed to get pending sync:" << query.lastError().text();
        return papers;
    }

    while (query.next()) {
        papers.append(fromQuery(query));
    }

    return papers;
}

bool LocalDatabase::updateSyncStatus(int paperId, const QString& status) {
    QWriteLocker locker(&lock_);

    QSqlQuery query(db_);
    query.prepare("UPDATE papers SET sync_status = ? WHERE id = ?");
    query.addBindValue(status);
    query.addBindValue(paperId);

    return query.exec();
}

bool LocalDatabase::markForSync(int paperId) {
    return updateSyncStatus(paperId, "pending");
}

QDateTime LocalDatabase::getLastSyncTime() {
    QReadLocker locker(&lock_);

    QSqlQuery query(
        "SELECT MAX(updated_at) FROM papers WHERE sync_status = 'synced'",
        db_
    );

    if (!query.exec() || !query.next()) {
        return QDateTime();
    }

    return query.value(0).toDateTime();
}

bool LocalDatabase::vacuum() {
    QWriteLocker locker(&lock_);

    QSqlQuery query("VACUUM", db_);
    if (!query.exec()) {
        qCritical() << "VACUUM failed:" << query.lastError().text();
        return false;
    }

    qDebug() << "Database vacuumed successfully";
    return true;
}

bool LocalDatabase::backup(const QString& backupPath) {
    QReadLocker locker(&lock_);

    // 使用 SQLite API 备份
    QSqlQuery query(db_);
    query.prepare(QString("VACUUM INTO '%1'").arg(backupPath));

    if (!query.exec()) {
        QString error = query.lastError().text();
        qCritical() << "Backup failed:" << error;
        emit databaseError(error);
        return false;
    }

    qDebug() << "Database backed up to:" << backupPath;
    return true;
}

QString LocalDatabase::generateTitleHash(const QString& title) {
    QByteArray hash = QCryptographicHash::hash(
        title.toUtf8().trimmed().toLower(),
        QCryptographicHash::Sha256
    );
    return hash.toHex();
}

DbPaper LocalDatabase::fromQuery(QSqlQuery& query) {
    DbPaper paper;
    paper.id = query.value("id").toInt();
    paper.doi = query.value("doi").toString();
    paper.title = query.value("title").toString();
    paper.authors = query.value("authors").toString();
    paper.abstract = query.value("abstract").toString();
    paper.journal = query.value("journal").toString();
    paper.year = query.value("year").toString();
    paper.volume = query.value("volume").toString();
    paper.issue = query.value("issue").toString();
    paper.pages = query.value("pages").toString();
    paper.keywords = query.value("keywords").toString();
    paper.pdfUrl = query.value("pdf_url").toString();
    paper.source = query.value("source").toString();
    paper.createdAt = query.value("created_at").toDateTime();
    paper.updatedAt = query.value("updated_at").toDateTime();
    paper.syncStatus = query.value("sync_status").toString();

    return paper;
}
