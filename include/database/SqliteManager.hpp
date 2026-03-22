#pragma once

#include <sqlite3.h>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <atomic>
#include <optional>

#include "models/Paper.hpp"
#include "models/Journal.hpp"

namespace PaperCrawler {

/**
 * @brief SQLite result row
 */
struct SqliteRow {
    std::vector<std::string> columns;

    std::string getString(int index) const {
        return index < columns.size() ? columns[index] : "";
    }

    int getInt(int index) const {
        try {
            return index < columns.size() ? std::stoi(columns[index]) : 0;
        } catch (...) {
            return 0;
        }
    }

    int64_t getInt64(int index) const {
        try {
            return index < columns.size() ? std::stoll(columns[index]) : 0;
        } catch (...) {
            return 0;
        }
    }

    double getDouble(int index) const {
        try {
            return index < columns.size() ? std::stod(columns[index]) : 0.0;
        } catch (...) {
            return 0.0;
        }
    }

    bool getBool(int index) const {
        return getInt(index) != 0;
    }
};

/**
 * @brief SQLite query result
 */
struct SqliteResult {
    std::vector<SqliteRow> rows;
    int lastInsertId{0};
    int rowsAffected{0};

    bool empty() const { return rows.empty(); }
    size_t size() const { return rows.size(); }
    const SqliteRow& operator[](size_t index) const { return rows[index]; }
};

/**
 * @brief Database connection configuration
 */
struct SqliteConfig {
    std::string databasePath{"papercrawler.db"};
    int cacheSize{-2000};                      // Negative = KB, positive = pages
    int journalMode{2};                         // WAL mode
    int synchronousMode{1};                     // NORMAL
    int mmapSize{30000000000};                  // 300GB max
    int pageSize{4096};
    int busyTimeout{5000};                      // 5 seconds
    int maxConnections{10};                     // Connection pool size
    bool foreignKeys{true};
    bool enableFTS{true};                       // Full-text search
};

/**
 * @brief Prepared statement cache entry
 */
struct PreparedStatement {
    sqlite3_stmt* stmt{nullptr};
    std::string sql;
    int lastUsed{0};                            // Timestamp for LRU eviction

    ~PreparedStatement() {
        if (stmt) {
            sqlite3_finalize(stmt);
        }
    }
};

/**
 * @brief SQLite database connection (single connection in pool)
 */
class SqliteConnection {
public:
    explicit SqliteConnection(const std::string& path);
    ~SqliteConnection();

    SqliteConnection(const SqliteConnection&) = delete;
    SqliteConnection& operator=(const SqliteConnection&) = delete;

    sqlite3* get() { return db_; }
    bool isValid() const { return db_ != nullptr; }

    // Execute SQL without returning results
    bool execute(const std::string& sql);

    // Query and return results
    SqliteResult query(const std::string& sql);

    // Get last insert row ID
    int64_t lastInsertId();

    // Get rows affected by last statement
    int rowsAffected();

    // Begin transaction
    bool begin();

    // Commit transaction
    bool commit();

    // Rollback transaction
    bool rollback();

    // Check if in transaction
    bool isInTransaction() const;

private:
    sqlite3* db_{nullptr};
    std::string path_;
};

/**
 * @brief Connection pool for SQLite
 */
class SqliteConnectionPool {
public:
    explicit SqliteConnectionPool(const SqliteConfig& config);
    ~SqliteConnectionPool();

    // Acquire connection from pool
    std::shared_ptr<SqliteConnection> acquire();

    // Return connection to pool
    void release(std::shared_ptr<SqliteConnection> conn);

    // Close all connections
    void close();

    size_t getPoolSize() const { return config_.maxConnections; }
    size_t getActiveCount() const { return activeCount_; }

private:
    SqliteConfig config_;
    std::vector<std::shared_ptr<SqliteConnection>> connections_;
    std::queue<std::shared_ptr<SqliteConnection>> available_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t activeCount_{0};
    bool closed_{false};
};

/**
 * @brief Main SQLite database manager with connection pooling
 */
class SqliteManager {
public:
    /**
     * @brief Get singleton instance
     */
    static SqliteManager& getInstance();

    /**
     * @brief Initialize database with configuration
     * @param config Database configuration
     * @return true if successful
     */
    bool initialize(const SqliteConfig& config = SqliteConfig{});

    /**
     * @brief Shutdown database and close connections
     */
    void shutdown();

    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return initialized_; }

    // ========== Basic Operations ==========

    /**
     * @brief Execute SQL statement (no results)
     */
    bool execute(const std::string& sql);

    /**
     * @brief Execute query and return results
     */
    SqliteResult query(const std::string& sql);

    /**
     * @brief Execute prepared statement
     */
    SqliteResult executePrepared(const std::string& sql,
                                  const std::vector<std::string>& params);

    /**
     * @brief Execute in transaction
     */
    bool transaction(std::function<bool(SqliteManager&)> callback);

    // ========== Paper Operations ==========

    /**
     * @brief Insert or replace paper
     */
    bool insertPaper(const Paper& paper);

    /**
     * @brief Batch insert papers (optimized)
     */
    int insertPapers(const std::vector<Paper>& papers);

    /**
     * @brief Update paper
     */
    bool updatePaper(const Paper& paper);

    /**
     * @brief Delete paper by ID
     */
    bool deletePaper(int id);

    /**
     * @brief Get paper by ID
     */
    std::optional<Paper> getPaper(int id);

    /**
     * @brief Get paper by server ID
     */
    std::optional<Paper> getPaperByServerId(int serverId);

    /**
     * @brief Search papers with filters
     */
    std::vector<Paper> searchPapers(const std::string& keyword,
                                     int year = 0,
                                     const std::string& level = "",
                                     int offset = 0,
                                     int limit = 20);

    /**
     * @brief Full-text search using FTS5
     */
    std::vector<Paper> fullTextSearch(const std::string& query,
                                       int offset = 0,
                                       int limit = 20);

    /**
     * @brief Get papers needing sync
     */
    std::vector<Paper> getPapersNeedingSync();

    /**
     * @brief Get papers by sync status
     */
    std::vector<Paper> getPapersBySyncStatus(const std::string& status);

    /**
     * @brief Mark paper for sync
     */
    bool markPaperForSync(int paperId);

    /**
     * @brief Update sync status
     */
    bool updateSyncStatus(int paperId, const std::string& status,
                          int64_t serverId = 0);

    // ========== Journal Operations ==========

    /**
     * @brief Insert or replace journal
     */
    bool insertJournal(const Journal& journal);

    /**
     * @brief Get journal by ID
     */
    std::optional<Journal> getJournal(int id);

    /**
     * @brief Get journal by server ID
     */
    std::optional<Journal> getJournalByServerId(int serverId);

    /**
     * @brief Get all journals
     */
    std::vector<Journal> getAllJournals();

    // ========== Search History ==========

    /**
     * @brief Record search query
     */
    bool recordSearch(const std::string& keyword,
                      const std::string& searchType,
                      int resultCount,
                      int durationMs);

    /**
     * @brief Get recent searches
     */
    std::vector<std::string> getRecentSearches(int limit = 10);

    /**
     * @brief Get popular searches
     */
    std::vector<std::pair<std::string, int>> getPopularSearches(int limit = 10);

    // ========== Statistics ==========

    /**
     * @brief Get database statistics
     */
    struct Statistics {
        int totalPapers{0};
        int totalJournals{0};
        int bookmarkedPapers{0};
        int downloadedPapers{0};
        int64_t databaseSize{0};
        std::map<std::string, int> papersByLevel;
        std::map<int, int> papersByYear;
    };

    Statistics getStatistics();

    /**
     * @brief Optimize database (VACUUM, ANALYZE)
     */
    bool optimize();

    /**
     * @brief Get database size in bytes
     */
    int64_t getDatabaseSize();

    // ========== Maintenance ==========

    /**
     * @brief Backup database to file
     */
    bool backup(const std::string& backupPath);

    /**
     * @brief Restore database from backup
     */
    bool restore(const std::string& backupPath);

    /**
     * @brief Check database integrity
     */
    bool checkIntegrity();

    /**
     * @brief Clear old cache entries
     */
    int clearOldCache(int daysOld = 30);

    // ========== Schema Management ==========

    /**
     * @brief Get current schema version
     */
    int getSchemaVersion();

    /**
     * @brief Set schema version
     */
    bool setSchemaVersion(int version);

    /**
     * @brief Run database migrations
     */
    bool runMigrations();

    // ========== Connection Pool ==========

    /**
     * @brief Get connection pool
     */
    std::shared_ptr<SqliteConnectionPool> getPool() { return pool_; }

    /**
     * @brief Execute with connection from pool
     */
    template<typename Func>
    auto withConnection(Func&& func) -> decltype(func(std::declval<SqliteConnection&>())) {
        auto conn = pool_->acquire();
        return func(*conn);
    }

private:
    SqliteManager() = default;
    ~SqliteManager();

    // Prevent copying
    SqliteManager(const SqliteManager&) = delete;
    SqliteManager& operator=(const SqliteManager&) = delete;

    // Helper methods
    std::shared_ptr<SqliteConnection> acquireConnection();
    void releaseConnection(std::shared_ptr<SqliteConnection> conn);

    // Paper serialization
    Paper rowToPaper(const SqliteRow& row);
    SqliteRow paperToRow(const Paper& paper);

    // Journal serialization
    Journal rowToJournal(const SqliteRow& row);

    bool initialized_{false};
    SqliteConfig config_;
    std::shared_ptr<SqliteConnectionPool> pool_;
    std::mutex mutex_;
};

/**
 * @brief RAII transaction helper
 */
class SqliteTransaction {
public:
    explicit SqliteTransaction(SqliteManager& manager)
        : manager_(manager), committed_(false) {
        manager_.execute("BEGIN");
    }

    ~SqliteTransaction() {
        if (!committed_) {
            manager_.execute("ROLLBACK");
        }
    }

    void commit() {
        manager_.execute("COMMIT");
        committed_ = true;
    }

    void rollback() {
        manager_.execute("ROLLBACK");
        committed_ = true;
    }

private:
    SqliteManager& manager_;
    bool committed_;
};

} // namespace PaperCrawler
