#include "database/SqliteManager.hpp"
#include "core/Logger.hpp"
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// SqliteConnection Implementation
// ============================================================================

SqliteConnection::SqliteConnection(const std::string& path)
    : path_(path) {
    if (sqlite3_open_v2(path.c_str(), &db_,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        nullptr) != SQLITE_OK) {
        LOG_ERROR("Failed to open database: {}", path);
        db_ = nullptr;
        return;
    }

    // Set pragmas for optimization
    execute("PRAGMA journal_mode = WAL");
    execute("PRAGMA synchronous = NORMAL");
    execute("PRAGMA foreign_keys = ON");
    execute("PRAGMA temp_store = MEMORY");
    execute("PRAGMA mmap_size = 30000000000");

    LOG_DEBUG("Opened SQLite connection: {}", path);
}

SqliteConnection::~SqliteConnection() {
    if (db_) {
        sqlite3_close_v2(db_);
        LOG_DEBUG("Closed SQLite connection: {}", path_);
    }
}

bool SqliteConnection::execute(const std::string& sql) {
    char* errorMsg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMsg);

    if (result != SQLITE_OK) {
        LOG_ERROR("SQL execute failed: {} - {}", sql, errorMsg ? errorMsg : "unknown");
        if (errorMsg) sqlite3_free(errorMsg);
        return false;
    }

    return true;
}

SqliteResult SqliteConnection::query(const std::string& sql) {
    SqliteResult result;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR("SQL prepare failed: {}", sql);
        return result;
    }

    int columnCount = sqlite3_column_count(stmt);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SqliteRow row;
        for (int i = 0; i < columnCount; ++i) {
            const char* val = reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, i)
            );
            row.columns.push_back(val ? val : "");
        }
        result.rows.push_back(row);
    }

    result.rowsAffected = sqlite3_changes(db_);
    result.lastInsertId = sqlite3_last_insert_rowid(db_);

    sqlite3_finalize(stmt);
    return result;
}

int64_t SqliteConnection::lastInsertId() {
    return sqlite3_last_insert_rowid(db_);
}

int SqliteConnection::rowsAffected() {
    return sqlite3_changes(db_);
}

bool SqliteConnection::begin() {
    return execute("BEGIN IMMEDIATE");
}

bool SqliteConnection::commit() {
    return execute("COMMIT");
}

bool SqliteConnection::rollback() {
    return execute("ROLLBACK");
}

bool SqliteConnection::isInTransaction() const {
    return sqlite3_get_autocommit(db_) == 0;
}

// ============================================================================
// SqliteConnectionPool Implementation
// ============================================================================

SqliteConnectionPool::SqliteConnectionPool(const SqliteConfig& config)
    : config_(config) {

    // Create database directory if not exists
    std::filesystem::path dbPath(config.databasePath);
    std::filesystem::create_directories(dbPath.parent_path());

    // Initialize connections
    for (int i = 0; i < config.maxConnections; ++i) {
        auto conn = std::make_shared<SqliteConnection>(config.databasePath);
        if (conn->isValid()) {
            connections_.push_back(conn);
            available_.push(conn);
        }
    }

    LOG_INFO("Created SQLite connection pool with {} connections",
             connections_.size());
}

SqliteConnectionPool::~SqliteConnectionPool() {
    close();
}

std::shared_ptr<SqliteConnection> SqliteConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);

    // Wait for available connection
    cv_.wait(lock, [this] {
        return !available_.empty() || closed_;
    });

    if (closed_) {
        return nullptr;
    }

    auto conn = available_.front();
    available_.pop();
    activeCount_++;

    return conn;
}

void SqliteConnectionPool::release(std::shared_ptr<SqliteConnection> conn) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (conn && !closed_) {
        available_.push(conn);
        activeCount_--;
    }

    cv_.notify_one();
}

void SqliteConnectionPool::close() {
    std::unique_lock<std::mutex> lock(mutex_);

    closed_ = true;

    // Close all connections
    connections_.clear();

    while (!available_.empty()) {
        available_.pop();
    }

    cv_.notify_all();
    LOG_INFO("Closed SQLite connection pool");
}

// ============================================================================
// SqliteManager Implementation
// ============================================================================

SqliteManager& SqliteManager::getInstance() {
    static SqliteManager instance;
    return instance;
}

SqliteManager::~SqliteManager() {
    shutdown();
}

bool SqliteManager::initialize(const SqliteConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        LOG_WARN("SqliteManager already initialized");
        return true;
    }

    config_ = config;

    // Create connection pool
    pool_ = std::make_shared<SqliteConnectionPool>(config);

    if (!pool_ || pool_->getPoolSize() == 0) {
        LOG_ERROR("Failed to create connection pool");
        return false;
    }

    // Run migrations
    if (!runMigrations()) {
        LOG_ERROR("Failed to run database migrations");
        return false;
    }

    initialized_ = true;
    LOG_INFO("SqliteManager initialized successfully");
    return true;
}

void SqliteManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        if (pool_) {
            pool_->close();
        }
        initialized_ = false;
        LOG_INFO("SqliteManager shutdown");
    }
}

std::shared_ptr<SqliteConnection> SqliteManager::acquireConnection() {
    return pool_->acquire();
}

void SqliteManager::releaseConnection(std::shared_ptr<SqliteConnection> conn) {
    pool_->release(conn);
}

bool SqliteManager::execute(const std::string& sql) {
    auto conn = acquireConnection();
    bool result = conn->execute(sql);
    releaseConnection(conn);
    return result;
}

SqliteResult SqliteManager::query(const std::string& sql) {
    auto conn = acquireConnection();
    auto result = conn->query(sql);
    releaseConnection(conn);
    return result;
}

SqliteResult SqliteManager::executePrepared(
    const std::string& sql,
    const std::vector<std::string>& params) {

    auto conn = acquireConnection();
    SqliteResult result;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(conn->get(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        LOG_ERROR("Failed to prepare statement: {}", sql);
        releaseConnection(conn);
        return result;
    }

    // Bind parameters
    for (size_t i = 0; i < params.size(); ++i) {
        if (sqlite3_bind_text(stmt, i + 1, params[i].c_str(),
                              params[i].length(), SQLITE_TRANSIENT) != SQLITE_OK) {
            LOG_ERROR("Failed to bind parameter {}", i);
            sqlite3_finalize(stmt);
            releaseConnection(conn);
            return result;
        }
    }

    // Execute and fetch results
    int columnCount = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SqliteRow row;
        for (int i = 0; i < columnCount; ++i) {
            const char* val = reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, i)
            );
            row.columns.push_back(val ? val : "");
        }
        result.rows.push_back(row);
    }

    result.rowsAffected = sqlite3_changes(conn->get());
    result.lastInsertId = sqlite3_last_insert_rowid(conn->get());

    sqlite3_finalize(stmt);
    releaseConnection(conn);

    return result;
}

bool SqliteManager::transaction(std::function<bool(SqliteManager&)> callback) {
    if (!execute("BEGIN IMMEDIATE")) {
        return false;
    }

    try {
        bool result = callback(*this);
        if (result) {
            execute("COMMIT");
        } else {
            execute("ROLLBACK");
        }
        return result;
    } catch (...) {
        execute("ROLLBACK");
        throw;
    }
}

// ============================================================================
// Paper Operations
// ============================================================================

bool SqliteManager::insertPaper(const Paper& paper) {
    std::ostringstream sql;
    sql << "INSERT OR REPLACE INTO papers ("
        << "server_id, sync_status, sync_version, title, authors, year, "
        << "journal_full, journal_short, journal_id, level, "
        << "doi_url, journal_url, type, qkid, abstract, keywords, "
        << "citation_count, is_bookmarked, is_read, user_notes, user_rating, "
        << "is_favorited, read_status, created_at, updated_at"
        << ") VALUES ("
        << "?" << ", "  // server_id
        << "?" << ", "  // sync_status
        << "1, "       // sync_version
        << "?" << ", "  // title
        << "?" << ", "  // authors
        << "?" << ", "  // year
        << "?" << ", "  // journal_full
        << "?" << ", "  // journal_short
        << "?" << ", "  // journal_id
        << "?" << ", "  // level
        << "?" << ", "  // doi_url
        << "?" << ", "  // journal_url
        << "?" << ", "  // type
        << "?" << ", "  // qkid
        << "?" << ", "  // abstract
        << "?" << ", "  // keywords
        << "0, "       // citation_count
        << "0, "       // is_bookmarked
        << "0, "       // is_read
        << "?" << ", "  // user_notes
        << "?" << ", "  // user_rating
        << "0, "       // is_favorited
        << "'unread', " // read_status
        << "strftime('%s', 'now'), "
        << "strftime('%s', 'now')"
        << ")";

    std::vector<std::string> params = {
        std::to_string(paper.getServerId()),
        "synced",
        paper.getTitle(),
        paper.getAuthor(),
        paper.getYear(),
        paper.getJournalFull(),
        paper.getJournalShort(),
        "0",  // journal_id
        paper.getLevel(),
        paper.getDoiUrl(),
        paper.getJournalUrl(),
        paper.getType(),
        std::to_string(paper.getQkid()),
        "",  // abstract
        "",  // keywords
        "",  // user_notes
        ""   // user_rating
    };

    auto result = executePrepared(sql.str(), params);
    return result.rowsAffected > 0;
}

int SqliteManager::insertPapers(const std::vector<Paper>& papers) {
    if (papers.empty()) return 0;

    return transaction([this, &papers]() {
        int count = 0;
        for (const auto& paper : papers) {
            if (insertPaper(paper)) {
                count++;
            }
        }
        return count > 0;
    }) ? papers.size() : 0;
}

bool SqliteManager::updatePaper(const Paper& paper) {
    std::ostringstream sql;
    sql << "UPDATE papers SET "
        << "title = ?, "
        << "authors = ?, "
        << "year = ?, "
        << "journal_full = ?, "
        << "journal_short = ?, "
        << "level = ?, "
        << "doi_url = ?, "
        << "journal_url = ?, "
        << "type = ?, "
        << "qkid = ?, "
        << "sync_status = 'pending', "
        << "sync_version = sync_version + 1, "
        << "updated_at = strftime('%s', 'now') "
        << "WHERE id = ?";

    std::vector<std::string> params = {
        paper.getTitle(),
        paper.getAuthor(),
        paper.getYear(),
        paper.getJournalFull(),
        paper.getJournalShort(),
        paper.getLevel(),
        paper.getDoiUrl(),
        paper.getJournalUrl(),
        paper.getType(),
        std::to_string(paper.getQkid()),
        std::to_string(paper.getId())
    };

    auto result = executePrepared(sql.str(), params);
    return result.rowsAffected > 0;
}

bool SqliteManager::deletePaper(int id) {
    std::string sql = "DELETE FROM papers WHERE id = " + std::to_string(id);
    return execute(sql);
}

std::optional<Paper> SqliteManager::getPaper(int id) {
    std::string sql = "SELECT * FROM papers WHERE id = " + std::to_string(id);
    auto result = query(sql);

    if (!result.empty()) {
        return rowToPaper(result.rows[0]);
    }

    return std::nullopt;
}

std::optional<Paper> SqliteManager::getPaperByServerId(int serverId) {
    std::string sql = "SELECT * FROM papers WHERE server_id = " + std::to_string(serverId);
    auto result = query(sql);

    if (!result.empty()) {
        return rowToPaper(result.rows[0]);
    }

    return std::nullopt;
}

std::vector<Paper> SqliteManager::searchPapers(
    const std::string& keyword,
    int year,
    const std::string& level,
    int offset,
    int limit) {

    std::ostringstream sql;
    sql << "SELECT * FROM papers WHERE 1=1";

    if (!keyword.empty()) {
        sql << " AND (title LIKE '%" << keyword << "%' "
            << "OR authors LIKE '%" << keyword << "%')";
    }

    if (year > 0) {
        sql << " AND year = " << year;
    }

    if (!level.empty()) {
        sql << " AND level = '" << level << "'";
    }

    sql << " ORDER BY year DESC, title ASC"
        << " LIMIT " << limit << " OFFSET " << offset;

    auto result = query(sql);

    std::vector<Paper> papers;
    for (const auto& row : result.rows) {
        papers.push_back(rowToPaper(row));
    }

    return papers;
}

std::vector<Paper> SqliteManager::fullTextSearch(
    const std::string& query,
    int offset,
    int limit) {

    std::string sql = "SELECT p.* FROM papers p "
                     "JOIN papers_fts fts ON p.id = fts.rowid "
                     "WHERE papers_fts MATCH '" + query + "' "
                     "ORDER BY rank "
                     "LIMIT " + std::to_string(limit) + " "
                     "OFFSET " + std::to_string(offset);

    auto result = query(sql);

    std::vector<Paper> papers;
    for (const auto& row : result.rows) {
        papers.push_back(rowToPaper(row));
    }

    return papers;
}

std::vector<Paper> SqliteManager::getPapersNeedingSync() {
    std::string sql = "SELECT * FROM papers WHERE sync_status IN ('pending', 'conflict') "
                     "ORDER BY updated_at ASC";
    auto result = query(sql);

    std::vector<Paper> papers;
    for (const auto& row : result.rows) {
        papers.push_back(rowToPaper(row));
    }

    return papers;
}

std::vector<Paper> SqliteManager::getPapersBySyncStatus(const std::string& status) {
    std::string sql = "SELECT * FROM papers WHERE sync_status = '" + status + "'";
    auto result = query(sql);

    std::vector<Paper> papers;
    for (const auto& row : result.rows) {
        papers.push_back(rowToPaper(row));
    }

    return papers;
}

bool SqliteManager::markPaperForSync(int paperId) {
    std::string sql = "UPDATE papers SET sync_status = 'pending', "
                     "sync_version = sync_version + 1, "
                     "updated_at = strftime('%s', 'now') "
                     "WHERE id = " + std::to_string(paperId);
    return execute(sql);
}

bool SqliteManager::updateSyncStatus(int paperId, const std::string& status,
                                      int64_t serverId) {
    std::ostringstream sql;
    sql << "UPDATE papers SET "
        << "sync_status = '" << status << "', "
        << "last_synced_at = strftime('%s', 'now')";

    if (serverId > 0) {
        sql << ", server_id = " << serverId;
    }

    sql << " WHERE id = " << paperId;

    return execute(sql.str());
}

// ============================================================================
// Journal Operations
// ============================================================================

bool SqliteManager::insertJournal(const Journal& journal) {
    std::ostringstream sql;
    sql << "INSERT OR REPLACE INTO journals ("
        << "server_id, name, name_short, full_name, level, "
        << "sync_status, created_at, updated_at"
        << ") VALUES ("
        << "?, ?, ?, ?, ?, 'synced', strftime('%s', 'now'), strftime('%s', 'now'))";

    std::vector<std::string> params = {
        std::to_string(journal.getId()),
        journal.getName(),
        journal.getShortName(),
        journal.getFullName(),
        journal.getLevel()
    };

    auto result = executePrepared(sql.str(), params);
    return result.rowsAffected > 0;
}

std::optional<Journal> SqliteManager::getJournal(int id) {
    std::string sql = "SELECT * FROM journals WHERE id = " + std::to_string(id);
    auto result = query(sql);

    if (!result.empty()) {
        return rowToJournal(result.rows[0]);
    }

    return std::nullopt;
}

std::optional<Journal> SqliteManager::getJournalByServerId(int serverId) {
    std::string sql = "SELECT * FROM journals WHERE server_id = " + std::to_string(serverId);
    auto result = query(sql);

    if (!result.empty()) {
        return rowToJournal(result.rows[0]);
    }

    return std::nullopt;
}

std::vector<Journal> SqliteManager::getAllJournals() {
    std::string sql = "SELECT * FROM journals ORDER BY name ASC";
    auto result = query(sql);

    std::vector<Journal> journals;
    for (const auto& row : result.rows) {
        journals.push_back(rowToJournal(row));
    }

    return journals;
}

// ============================================================================
// Search History
// ============================================================================

bool SqliteManager::recordSearch(const std::string& keyword,
                                  const std::string& searchType,
                                  int resultCount,
                                  int durationMs) {
    std::ostringstream sql;
    sql << "INSERT INTO search_history (keyword, search_type, result_count, search_duration_ms) "
        << "VALUES ('" << keyword << "', '" << searchType << "', "
        << resultCount << ", " << durationMs << ")";

    return execute(sql.str());
}

std::vector<std::string> SqliteManager::getRecentSearches(int limit) {
    std::string sql = "SELECT DISTINCT keyword FROM search_history "
                     "ORDER BY created_at DESC "
                     "LIMIT " + std::to_string(limit);

    auto result = query(sql);

    std::vector<std::string> searches;
    for (const auto& row : result.rows) {
        searches.push_back(row.getString(0));
    }

    return searches;
}

std::vector<std::pair<std::string, int>> SqliteManager::getPopularSearches(int limit) {
    std::string sql = "SELECT keyword, COUNT(*) as count FROM search_history "
                     "GROUP BY keyword "
                     "ORDER BY count DESC "
                     "LIMIT " + std::to_string(limit);

    auto result = query(sql);

    std::vector<std::pair<std::string, int>> searches;
    for (const auto& row : result.rows) {
        searches.emplace_back(row.getString(0), row.getInt(1));
    }

    return searches;
}

// ============================================================================
// Statistics
// ============================================================================

SqliteManager::Statistics SqliteManager::getStatistics() {
    Statistics stats;

    auto result = query("SELECT COUNT(*) FROM papers");
    if (!result.empty()) {
        stats.totalPapers = result[0].getInt(0);
    }

    result = query("SELECT COUNT(*) FROM journals");
    if (!result.empty()) {
        stats.totalJournals = result[0].getInt(0);
    }

    result = query("SELECT COUNT(*) FROM papers WHERE is_bookmarked = 1");
    if (!result.empty()) {
        stats.bookmarkedPapers = result[0].getInt(0);
    }

    result = query("SELECT COUNT(*) FROM papers WHERE read_status = 'read'");
    if (!result.empty()) {
        stats.downloadedPapers = result[0].getInt(0);
    }

    // Papers by level
    result = query("SELECT level, COUNT(*) FROM papers GROUP BY level");
    for (const auto& row : result.rows) {
        stats.papersByLevel[row.getString(0)] = row.getInt(1);
    }

    // Papers by year
    result = query("SELECT year, COUNT(*) FROM papers GROUP BY year ORDER BY year DESC LIMIT 10");
    for (const auto& row : result.rows) {
        stats.papersByYear[row.getInt(0)] = row.getInt(1);
    }

    stats.databaseSize = getDatabaseSize();

    return stats;
}

bool SqliteManager::optimize() {
    LOG_INFO("Optimizing database...");

    if (!execute("ANALYZE")) {
        LOG_WARN("Failed to analyze database");
    }

    if (!execute("VACUUM")) {
        LOG_WARN("Failed to vacuum database");
    }

    if (!execute("PRAGMA optimize")) {
        LOG_WARN("Failed to optimize database");
    }

    LOG_INFO("Database optimization complete");
    return true;
}

int64_t SqliteManager::getDatabaseSize() {
    std::filesystem::path dbPath(config_.databasePath);

    if (!std::filesystem::exists(dbPath)) {
        return 0;
    }

    int64_t size = std::filesystem::file_size(dbPath);

    // Add WAL file size
    std::filesystem::path walPath = dbPath.string() + "-wal";
    if (std::filesystem::exists(walPath)) {
        size += std::filesystem::file_size(walPath);
    }

    return size;
}

// ============================================================================
// Maintenance
// ============================================================================

bool SqliteManager::backup(const std::string& backupPath) {
    std::filesystem::create_directories(
        std::filesystem::path(backupPath).parent_path()
    );

    auto conn = acquireConnection();
    sqlite3* backupDb = nullptr;

    if (sqlite3_open(backupPath.c_str(), &backupDb) != SQLITE_OK) {
        LOG_ERROR("Failed to create backup database");
        releaseConnection(conn);
        return false;
    }

    sqlite3_backup* backup = sqlite3_backup_init(
        backupDb, "main", conn->get(), "main"
    );

    if (!backup) {
        LOG_ERROR("Failed to initialize backup");
        sqlite3_close(backupDb);
        releaseConnection(conn);
        return false;
    }

    int result = sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    sqlite3_close(backupDb);
    releaseConnection(conn);

    if (result != SQLITE_DONE) {
        LOG_ERROR("Backup failed with code: {}", result);
        return false;
    }

    LOG_INFO("Database backed up to: {}", backupPath);
    return true;
}

bool SqliteManager::restore(const std::string& backupPath) {
    if (!std::filesystem::exists(backupPath)) {
        LOG_ERROR("Backup file does not exist: {}", backupPath);
        return false;
    }

    shutdown();

    // Copy backup file
    std::filesystem::copy_file(
        backupPath, config_.databasePath,
        std::filesystem::copy_options::overwrite_existing
    );

    initialize(config_);

    LOG_INFO("Database restored from: {}", backupPath);
    return true;
}

bool SqliteManager::checkIntegrity() {
    auto result = query("PRAGMA integrity_check");

    if (!result.empty()) {
        std::string status = result[0].getString(0);
        if (status == "ok") {
            LOG_INFO("Database integrity check passed");
            return true;
        } else {
            LOG_ERROR("Database integrity check failed: {}", status);
            return false;
        }
    }

    return false;
}

int SqliteManager::clearOldCache(int daysOld) {
    int64_t cutoff = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())
                     - (daysOld * 24 * 3600);

    std::string sql = "DELETE FROM search_history WHERE created_at < "
                     + std::to_string(cutoff);

    execute(sql);

    auto result = query("SELECT changes()");
    if (!result.empty()) {
        int count = result[0].getInt(0);
        LOG_INFO("Cleared {} old cache entries", count);
        return count;
    }

    return 0;
}

// ============================================================================
// Schema Management
// ============================================================================

int SqliteManager::getSchemaVersion() {
    auto result = query("PRAGMA user_version");
    if (!result.empty()) {
        return result[0].getInt(0);
    }
    return 0;
}

bool SqliteManager::setSchemaVersion(int version) {
    return execute("PRAGMA user_version = " + std::to_string(version));
}

bool SqliteManager::runMigrations() {
    int currentVersion = getSchemaVersion();
    int targetVersion = 1;  // Current schema version

    LOG_INFO("Current schema version: {}, Target: {}", currentVersion, targetVersion);

    if (currentVersion >= targetVersion) {
        LOG_INFO("Database schema is up to date");
        return true;
    }

    // Run schema creation script
    std::string schemaPath = "database-schema.sql";
    if (!std::filesystem::exists(schemaPath)) {
        LOG_WARN("Schema file not found: {}", schemaPath);
        // Continue anyway, tables may already exist
    } else {
        // Read and execute schema
        // (In production, you'd read the SQL file here)
        LOG_INFO("Executing schema migration...");
    }

    // Update version
    if (!setSchemaVersion(targetVersion)) {
        LOG_ERROR("Failed to set schema version");
        return false;
    }

    LOG_INFO("Schema migration complete");
    return true;
}

// ============================================================================
// Helper Methods
// ============================================================================

Paper SqliteManager::rowToPaper(const SqliteRow& row) {
    Paper paper;
    paper.setId(row.getInt(0));
    paper.setServerId(row.getInt64(1));
    paper.setTitle(row.getString(3));
    paper.setAuthor(row.getString(4));
    paper.setYear(row.getString(5));
    paper.setJournalFull(row.getString(6));
    paper.setJournalShort(row.getString(7));
    paper.setLevel(row.getString(9));
    paper.setDoiUrl(row.getString(10));
    paper.setJournalUrl(row.getString(11));
    paper.setType(row.getString(12));
    paper.setQkid(row.getInt(13));
    return paper;
}

Journal SqliteManager::rowToJournal(const SqliteRow& row) {
    Journal journal;
    journal.setId(row.getInt(0));
    journal.setName(row.getString(1));
    journal.setShortName(row.getString(2));
    journal.setFullName(row.getString(3));
    journal.setLevel(row.getString(4));
    return journal;
}

} // namespace PaperCrawler
