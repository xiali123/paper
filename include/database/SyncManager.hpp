#pragma once

#include "database/SqliteManager.hpp"
#include "models/Paper.hpp"
#include "models/Journal.hpp"
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>

namespace PaperCrawler {

/**
 * @brief Synchronization configuration
 */
struct SyncConfig {
    std::string serverUrl{"http://localhost:8080/api"};
    int syncIntervalSeconds{3600};              // Auto-sync every hour
    int batchSize{100};                         // Papers per batch
    int maxRetries{3};
    int retryDelayMs{1000};
    int timeoutSeconds{30};
    bool enableIncrementalSync{true};           // Use incremental sync
    bool conflictResolutionClientWins{false};   // Conflict resolution strategy
    bool compressData{true};                    // Compress sync payload
};

/**
 * @brief Synchronization result
 */
struct SyncResult {
    bool success{false};
    int papersUploaded{0};
    int papersDownloaded{0};
    int papersSkipped{0};
    int conflictsResolved{0};
    int errors{0};
    double durationSeconds{0};
    std::string errorMessage;
    std::vector<std::string> conflictDetails;
};

/**
 * @brief Sync state for tracking progress
 */
struct SyncState {
    std::atomic<bool> isSyncing{false};
    std::atomic<int> progressCurrent{0};
    std::atomic<int> progressTotal{0};
    std::string currentPhase;
    std::chrono::system_clock::time_point lastSyncTime;
    SyncResult lastResult;
};

/**
 * @brief Change tracking entry
 */
struct ChangeEntry {
    enum class Type { Insert, Update, Delete };
    Type type;
    std::string tableName;
    int64_t rowId;
    int64_t serverId;
    int version;
    std::string data;  // JSON serialized data
    int64_t timestamp;
};

/**
 * @brief Conflict resolution strategy
 */
enum class ConflictResolution {
    ClientWins,     // Client overwrites server
    ServerWins,     // Server overwrites client
    NewestWins,     // Newest timestamp wins
    Manual,         // Require manual resolution
    Merge           // Attempt intelligent merge
};

/**
 * @brief Sync callback interface
 */
class ISyncCallback {
public:
    virtual ~ISyncCallback() = default;

    virtual void onSyncStart() {}
    virtual void onSyncProgress(int current, int total, const std::string& phase) {}
    virtual void onSyncComplete(const SyncResult& result) {}
    virtual void onSyncError(const std::string& error) {}
    virtual void onConflictDetected(const std::string& details) {}
};

/**
 * @brief Incremental synchronization manager
 *
 * Implements bidirectional sync with conflict resolution:
 * 1. Track local changes via sync_status and sync_version
 * 2. Pull remote changes since last sync
 * 3. Push local changes to server
 * 4. Detect and resolve conflicts
 * 5. Update sync state
 */
class SyncManager {
public:
    /**
     * @brief Get singleton instance
     */
    static SyncManager& getInstance();

    /**
     * @brief Initialize sync manager
     * @param config Sync configuration
     */
    void initialize(const SyncConfig& config = SyncConfig{});

    /**
     * @brief Shutdown sync manager
     */
    void shutdown();

    /**
     * @brief Set sync callback
     */
    void setCallback(std::shared_ptr<ISyncCallback> callback);

    /**
     * @brief Perform full synchronization
     * @return Sync result
     */
    SyncResult sync();

    /**
     * @brief Perform incremental sync (since last sync)
     * @return Sync result
     */
    SyncResult incrementalSync();

    /**
     * @brief Pull changes from server
     * @return Sync result
     */
    SyncResult pull();

    /**
     * @brief Push local changes to server
     * @return Sync result
     */
    SyncResult push();

    /**
     * @brief Start auto-sync background thread
     */
    void startAutoSync();

    /**
     * @brief Stop auto-sync background thread
     */
    void stopAutoSync();

    /**
     * @brief Force immediate sync (resets auto-sync timer)
     */
    SyncResult syncNow();

    /**
     * @brief Get current sync state
     */
    SyncState getState() const { return state_; }

    /**
     * @brief Check if sync is in progress
     */
    bool isSyncing() const { return state_.isSyncing; }

    /**
     * @brief Get last sync result
     */
    SyncResult getLastResult() const { return state_.lastResult; }

    // ========== Conflict Resolution ==========

    /**
     * @brief Set conflict resolution strategy
     */
    void setConflictResolution(ConflictResolution strategy) {
        conflictResolution_ = strategy;
    }

    /**
     * @brief Resolve conflict manually
     * @param paperId Local paper ID
     * @param useClientData true to use client version, false for server
     */
    bool resolveConflict(int paperId, bool useClientData);

    // ========== Change Tracking ==========

    /**
     * @brief Mark paper for sync
     */
    bool markForSync(int paperId);

    /**
     * @brief Mark journal for sync
     */
    bool markJournalForSync(int journalId);

    /**
     * @brief Get all pending changes
     */
    std::vector<ChangeEntry> getPendingChanges();

    // ========== Statistics ==========

    /**
     * @brief Get sync statistics
     */
    struct SyncStats {
        int64_t lastSyncTimestamp{0};
        int pendingUploadCount{0};
        int pendingDownloadCount{0};
        int totalSyncCount{0};
        int conflictCount{0};
        double avgSyncDuration{0};
    };

    SyncStats getStats() const;

private:
    SyncManager() = default;
    ~SyncManager();

    // Prevent copying
    SyncManager(const SyncManager&) = delete;
    SyncManager& operator=(const SyncManager&) = delete;

    // ========== Implementation Details ==========

    /**
     * @brief Pull papers from server
     */
    std::vector<Paper> pullPapers(int64_t sinceTimestamp);

    /**
     * @brief Push papers to server
     */
    int pushPapers(const std::vector<Paper>& papers);

    /**
     * @brief Pull journals from server
     */
    std::vector<Journal> pullJournals(int64_t sinceTimestamp);

    /**
     * @brief Push journals to server
     */
    int pushJournals(const std::vector<Journal>& journals);

    /**
     * @brief Detect and resolve conflicts
     */
    bool detectAndResolveConflicts(
        const std::vector<Paper>& serverPapers,
        const std::vector<Paper>& localPapers
    );

    /**
     * @brief Resolve single conflict
     */
    bool resolveConflict(
        const Paper& serverPaper,
        const Paper& localPaper,
        ConflictResolution strategy
    );

    /**
     * @brief Apply server changes to local database
     */
    bool applyServerChanges(const std::vector<Paper>& papers);

    /**
     * @brief Update sync metadata
     */
    bool updateSyncMetadata(int64_t serverTimestamp);

    /**
     * @brief Get last sync timestamp
     */
    int64_t getLastSyncTimestamp();

    /**
     * @brief Set last sync timestamp
     */
    bool setLastSyncTimestamp(int64_t timestamp);

    /**
     * @brief Generate sync payload JSON
     */
    std::string generateSyncPayload(const std::vector<Paper>& papers);

    /**
     * @brief Parse sync response JSON
     */
    std::vector<Paper> parseSyncResponse(const std::string& json);

    /**
     * @brief Compress data
     */
    std::string compressData(const std::string& data);

    /**
     * @brief Decompress data
     */
    std::string decompressData(const std::string& compressed);

    /**
     * @brief Make HTTP request to server
     */
    std::string makeRequest(const std::string& endpoint,
                            const std::string& method = "GET",
                            const std::string& body = "");

    // ========== Auto-sync Thread ==========

    /**
     * @brief Auto-sync thread function
     */
    void autoSyncThread();

    // Member variables
    SyncConfig config_;
    SyncState state_;
    std::shared_ptr<ISyncCallback> callback_;
    ConflictResolution conflictResolution_{ConflictResolution::NewestWins};

    std::mutex mutex_;
    std::thread autoSyncThread_;
    std::atomic<bool> autoSyncEnabled_{false};
    std::atomic<bool> shouldStop_{false};

    bool initialized_{false};
};

/**
 * @brief RAII helper for automatic sync on data changes
 */
class ScopedSyncTrigger {
public:
    explicit ScopedSyncTrigger(bool trigger = true) : trigger_(trigger) {}

    ~ScopedSyncTrigger() {
        if (trigger_) {
            // Trigger sync in background
            std::thread([]() {
                SyncManager::getInstance().syncNow();
            }).detach();
        }
    }

private:
    bool trigger_;
};

/**
 * @brief Scoped lock that prevents sync during critical operations
 */
class SyncLock {
public:
    SyncLock() {
        SyncManager::getInstance().stopAutoSync();
    }

    ~SyncLock() {
        SyncManager::getInstance().startAutoSync();
    }
};

} // namespace PaperCrawler
