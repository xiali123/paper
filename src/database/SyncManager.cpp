#include "database/SyncManager.hpp"
#include "database/SqliteManager.hpp"
#include "core/Logger.hpp"
#include "network/HttpClient.hpp"
#include <nlohmann/json.hpp>
#include <thread>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <iomanip>

// Compression libraries (would use zlib in production)
// For now, we'll use placeholder implementations

namespace PaperCrawler {

// ============================================================================
// Singleton Instance
// ============================================================================

SyncManager& SyncManager::getInstance() {
    static SyncManager instance;
    return instance;
}

SyncManager::~SyncManager() {
    shutdown();
}

// ============================================================================
// Initialization
// ============================================================================

void SyncManager::initialize(const SyncConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        LOG_WARN("SyncManager already initialized");
        return;
    }

    config_ = config;

    // Load last sync timestamp from preferences
    auto lastSync = getLastSyncTimestamp();
    state_.lastSyncTime = std::chrono::system_clock::from_time_t(lastSync);

    initialized_ = true;
    LOG_INFO("SyncManager initialized: server={}, interval={}s",
             config_.serverUrl, config_.syncIntervalSeconds);
}

void SyncManager::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (autoSyncEnabled_) {
        stopAutoSync();
    }

    initialized_ = false;
    LOG_INFO("SyncManager shutdown");
}

void SyncManager::setCallback(std::shared_ptr<ISyncCallback> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = callback;
}

// ============================================================================
// Synchronization Operations
// ============================================================================

SyncResult SyncManager::sync() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        SyncResult result;
        result.success = false;
        result.errorMessage = "SyncManager not initialized";
        return result;
    }

    if (state_.isSyncing) {
        LOG_WARN("Sync already in progress");
        SyncResult result;
        result.success = false;
        result.errorMessage = "Sync already in progress";
        return result;
    }

    state_.isSyncing = true;
    auto startTime = std::chrono::high_resolution_clock::now();

    if (callback_) {
        callback_->onSyncStart();
    }

    SyncResult result;
    result.success = true;

    try {
        // Phase 1: Pull changes from server
        state_.currentPhase = "Pulling changes from server";
        state_.progressCurrent = 0;
        state_.progressTotal = 100;

        if (callback_) {
            callback_->onSyncProgress(0, 100, "Pulling changes");
        }

        auto pullResult = pull();
        result.papersDownloaded = pullResult.papersDownloaded;

        if (!pullResult.success) {
            result.success = false;
            result.errorMessage = "Pull failed: " + pullResult.errorMessage;
            LOG_ERROR("Pull failed: {}", pullResult.errorMessage);
        }

        // Phase 2: Push local changes to server
        if (result.success) {
            state_.currentPhase = "Pushing local changes";
            state_.progressCurrent = 50;

            if (callback_) {
                callback_->onSyncProgress(50, 100, "Pushing changes");
            }

            auto pushResult = push();
            result.papersUploaded = pushResult.papersUploaded;

            if (!pushResult.success) {
                result.success = false;
                result.errorMessage = "Push failed: " + pushResult.errorMessage;
                LOG_ERROR("Push failed: {}", pushResult.errorMessage);
            }
        }

        // Phase 3: Resolve conflicts
        if (result.success) {
            state_.currentPhase = "Resolving conflicts";
            state_.progressCurrent = 90;

            if (callback_) {
                callback_->onSyncProgress(90, 100, "Resolving conflicts");
            }

            // Conflicts are resolved during pull/push
        }

        // Update last sync timestamp
        if (result.success) {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::system_clock::to_time_t(now);
            setLastSyncTimestamp(timestamp);
            state_.lastSyncTime = now;
        }

        state_.progressCurrent = 100;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
        LOG_ERROR("Sync exception: {}", e.what());
    }

    // Calculate duration
    auto endTime = std::chrono::high_resolution_clock::now();
    result.durationSeconds = std::chrono::duration<double>(
        endTime - startTime
    ).count();

    state_.isSyncing = false;
    state_.lastResult = result;

    if (callback_) {
        if (result.success) {
            callback_->onSyncComplete(result);
        } else {
            callback_->onSyncError(result.errorMessage);
        }
    }

    LOG_INFO("Sync completed: uploaded={}, downloaded={}, conflicts={}, duration={:.2f}s",
             result.papersUploaded, result.papersDownloaded,
             result.conflictsResolved, result.durationSeconds);

    return result;
}

SyncResult SyncManager::incrementalSync() {
    std::lock_guard<std::mutex> lock(mutex_);

    auto lastTimestamp = getLastSyncTimestamp();
    LOG_INFO("Starting incremental sync since {}", lastTimestamp);

    // For incremental sync, we only fetch changes since last sync
    // This is handled by the pull() method with timestamp filter

    return sync();
}

SyncResult SyncManager::pull() {
    SyncResult result;

    try {
        auto lastTimestamp = getLastSyncTimestamp();

        // Pull papers
        auto papers = pullPapers(lastTimestamp);

        if (!papers.empty()) {
            // Apply server changes
            if (applyServerChanges(papers)) {
                result.papersDownloaded = papers.size();
                LOG_INFO("Pulled {} papers from server", papers.size());
            }
        } else {
            LOG_INFO("No new papers to pull from server");
        }

        // Pull journals
        auto journals = pullJournals(lastTimestamp);

        if (!journals.empty()) {
            auto& db = SqliteManager::getInstance();
            for (const auto& journal : journals) {
                db.insertJournal(journal);
            }
            LOG_INFO("Pulled {} journals from server", journals.size());
        }

        result.success = true;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
        LOG_ERROR("Pull failed: {}", e.what());
    }

    return result;
}

SyncResult SyncManager::push() {
    SyncResult result;

    try {
        auto& db = SqliteManager::getInstance();

        // Get papers needing sync
        auto papers = db.getPapersNeedingSync();

        if (papers.empty()) {
            LOG_INFO("No local changes to push");
            result.success = true;
            return result;
        }

        // Push in batches
        int pushed = 0;
        for (size_t i = 0; i < papers.size(); i += config_.batchSize) {
            size_t end = std::min(i + config_.batchSize, papers.size());
            std::vector<Paper> batch(papers.begin() + i, papers.begin() + end);

            int count = pushPapers(batch);
            pushed += count;

            if (count > 0) {
                // Mark as synced
                for (const auto& paper : batch) {
                    db.updateSyncStatus(paper.getId(), "synced");
                }
            }

            state_.progressCurrent = static_cast<int>(50 + 50 * end / papers.size());

            if (callback_) {
                callback_->onSyncProgress(
                    state_.progressCurrent,
                    100,
                    "Pushing batch " + std::to_string(i / config_.batchSize + 1)
                );
            }
        }

        result.papersUploaded = pushed;
        result.success = true;

        LOG_INFO("Pushed {} papers to server", pushed);

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
        LOG_ERROR("Push failed: {}", e.what());
    }

    return result;
}

// ============================================================================
// Auto-sync
// ============================================================================

void SyncManager::startAutoSync() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (autoSyncEnabled_) {
        LOG_WARN("Auto-sync already running");
        return;
    }

    autoSyncEnabled_ = true;
    shouldStop_ = false;

    autoSyncThread_ = std::thread(&SyncManager::autoSyncThread, this);

    LOG_INFO("Auto-sync started: interval={}s", config_.syncIntervalSeconds);
}

void SyncManager::stopAutoSync() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!autoSyncEnabled_) {
        return;
    }

    shouldStop_ = true;
    autoSyncEnabled_ = false;

    if (autoSyncThread_.joinable()) {
        autoSyncThread_.join();
    }

    LOG_INFO("Auto-sync stopped");
}

SyncResult SyncManager::syncNow() {
    // Reset auto-sync timer by updating last sync time
    auto now = std::chrono::system_clock::now();
    state_.lastSyncTime = now;

    return sync();
}

void SyncManager::autoSyncThread() {
    while (!shouldStop_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - state_.lastSyncTime
        ).count();

        if (elapsed >= config_.syncIntervalSeconds) {
            LOG_INFO("Auto-sync triggered");

            try {
                sync();
            } catch (const std::exception& e) {
                LOG_ERROR("Auto-sync failed: {}", e.what());
            }
        }
    }
}

// ============================================================================
// Conflict Resolution
// ============================================================================

bool SyncManager::resolveConflict(int paperId, bool useClientData) {
    auto& db = SqliteManager::getInstance();

    auto localPaper = db.getPaper(paperId);
    if (!localPaper) {
        LOG_ERROR("Paper not found: {}", paperId);
        return false;
    }

    // Fetch server version
    std::string endpoint = config_.serverUrl + "/papers/" +
                          std::to_string(localPaper->getServerId());

    std::string response = makeRequest(endpoint, "GET", "");
    if (response.empty()) {
        LOG_ERROR("Failed to fetch server paper");
        return false;
    }

    auto serverPapers = parseSyncResponse(response);
    if (serverPapers.empty()) {
        LOG_ERROR("No server paper data");
        return false;
    }

    const auto& serverPaper = serverPapers[0];

    // Apply resolution
    ConflictResolution strategy = useClientData ?
        ConflictResolution::ClientWins : ConflictResolution::ServerWins;

    return resolveConflict(serverPaper, *localPaper, strategy);
}

bool SyncManager::resolveConflict(const Paper& serverPaper,
                                   const Paper& localPaper,
                                   ConflictResolution strategy) {
    auto& db = SqliteManager::getInstance();

    try {
        switch (strategy) {
            case ConflictResolution::ClientWins:
                // Push local version to server
                updateSyncMetadata(0);
                break;

            case ConflictResolution::ServerWins:
                // Overwrite local with server version
                db.insertPaper(serverPaper);
                db.updateSyncStatus(localPaper.getId(), "synced",
                                    serverPaper.getServerId());
                break;

            case ConflictResolution::NewestWins: {
                // Compare timestamps
                auto localUpdated = localPaper.getUpdatedAt();
                auto serverUpdated = serverPaper.getUpdatedAt();

                if (localUpdated > serverUpdated) {
                    return resolveConflict(serverPaper, localPaper,
                                          ConflictResolution::ClientWins);
                } else {
                    return resolveConflict(serverPaper, localPaper,
                                          ConflictResolution::ServerWins);
                }
            }

            case ConflictResolution::Manual:
                // Can't resolve automatically
                if (callback_) {
                    std::ostringstream details;
                    details << "Conflict on paper: " << localPaper.getTitle()
                           << " (local ID: " << localPaper.getId()
                           << ", server ID: " << serverPaper.getServerId() << ")";
                    callback_->onConflictDetected(details.str());
                }
                return false;

            case ConflictResolution::Merge:
                // Attempt intelligent merge
                // For now, use server as base and merge non-conflicting fields
                Paper merged = serverPaper;
                // Add merge logic here
                db.insertPaper(merged);
                db.updateSyncStatus(localPaper.getId(), "synced",
                                    serverPaper.getServerId());
                break;
        }

        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Conflict resolution failed: {}", e.what());
        return false;
    }
}

// ============================================================================
// Server Communication
// ============================================================================

std::vector<Paper> SyncManager::pullPapers(int64_t sinceTimestamp) {
    try {
        std::ostringstream url;
        url << config_.serverUrl << "/sync/papers?since=" << sinceTimestamp;

        std::string response = makeRequest(url.str(), "GET", "");

        if (response.empty()) {
            return {};
        }

        return parseSyncResponse(response);

    } catch (const std::exception& e) {
        LOG_ERROR("Pull papers failed: {}", e.what());
        return {};
    }
}

int SyncManager::pushPapers(const std::vector<Paper>& papers) {
    try {
        std::string payload = generateSyncPayload(papers);

        if (config_.compressData) {
            payload = compressData(payload);
        }

        std::string response = makeRequest(
            config_.serverUrl + "/sync/papers",
            "POST",
            payload
        );

        if (response.empty()) {
            return 0;
        }

        auto json = nlohmann::json::parse(response);
        if (json.contains("success") && json["success"]) {
            return json.value("count", 0);
        }

        return 0;

    } catch (const std::exception& e) {
        LOG_ERROR("Push papers failed: {}", e.what());
        return 0;
    }
}

std::vector<Journal> SyncManager::pullJournals(int64_t sinceTimestamp) {
    try {
        std::ostringstream url;
        url << config_.serverUrl << "/sync/journals?since=" << sinceTimestamp;

        std::string response = makeRequest(url.str(), "GET", "");

        if (response.empty()) {
            return {};
        }

        auto json = nlohmann::json::parse(response);
        if (json.contains("journals")) {
            return json["journals"].get<std::vector<Journal>>();
        }

        return {};

    } catch (const std::exception& e) {
        LOG_ERROR("Pull journals failed: {}", e.what());
        return {};
    }
}

int SyncManager::pushJournals(const std::vector<Journal>& journals) {
    // Similar to pushPapers
    return 0;  // Placeholder
}

bool SyncManager::applyServerChanges(const std::vector<Paper>& papers) {
    auto& db = SqliteManager::getInstance();

    return db.transaction([&db, &papers]() {
        for (const auto& paper : papers) {
            // Check if local version exists
            auto localPaper = db.getPaperByServerId(paper.getServerId());

            if (localPaper) {
                // Check for conflict
                if (localPaper->getSyncVersion() != paper.getSyncVersion()) {
                    // Conflict detected - resolve using configured strategy
                    resolveConflict(paper, *localPaper, conflictResolution_);
                } else {
                    // No conflict - update from server
                    db.insertPaper(paper);
                    db.updateSyncStatus(localPaper->getId(), "synced",
                                        paper.getServerId());
                }
            } else {
                // New paper from server
                db.insertPaper(paper);
            }
        }
        return true;
    });
}

std::string SyncManager::generateSyncPayload(const std::vector<Paper>& papers) {
    nlohmann::json json;
    json["papers"] = nlohmann::json::array();

    for (const auto& paper : papers) {
        nlohmann::json p;
        p["id"] = paper.getId();
        p["server_id"] = paper.getServerId();
        p["title"] = paper.getTitle();
        p["authors"] = paper.getAuthor();
        p["year"] = paper.getYear();
        p["journal_full"] = paper.getJournalFull();
        p["journal_short"] = paper.getJournalShort();
        p["level"] = paper.getLevel();
        p["doi_url"] = paper.getDoiUrl();
        p["journal_url"] = paper.getJournalUrl();
        p["type"] = paper.getType();
        p["qkid"] = paper.getQkid();
        p["sync_version"] = paper.getSyncVersion();
        json["papers"].push_back(p);
    }

    return json.dump();
}

std::vector<Paper> SyncManager::parseSyncResponse(const std::string& jsonStr) {
    std::vector<Paper> papers;

    try {
        auto json = nlohmann::json::parse(jsonStr);

        if (json.contains("papers")) {
            for (const auto& item : json["papers"]) {
                Paper paper;
                paper.setServerId(item.value("server_id", 0));
                paper.setTitle(item.value("title", ""));
                paper.setAuthor(item.value("authors", ""));
                paper.setYear(item.value("year", ""));
                paper.setJournalFull(item.value("journal_full", ""));
                paper.setJournalShort(item.value("journal_short", ""));
                paper.setLevel(item.value("level", ""));
                paper.setDoiUrl(item.value("doi_url", ""));
                paper.setJournalUrl(item.value("journal_url", ""));
                paper.setType(item.value("type", ""));
                paper.setQkid(item.value("qkid", 0));
                papers.push_back(paper);
            }
        }

    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse sync response: {}", e.what());
    }

    return papers;
}

std::string SyncManager::compressData(const std::string& data) {
    // Placeholder - would use zlib in production
    // For now, just return base64 encoded
    return data;
}

std::string SyncManager::decompressData(const std::string& compressed) {
    // Placeholder - would use zlib in production
    return compressed;
}

std::string SyncManager::makeRequest(const std::string& endpoint,
                                      const std::string& method,
                                      const std::string& body) {
    try {
        HttpClient client(config_.timeoutSeconds);

        if (method == "GET") {
            auto response = client.get(endpoint);
            if (response.success) {
                return response.body;
            }
        } else if (method == "POST") {
            auto response = client.post(endpoint, body);
            if (response.success) {
                return response.body;
            }
        }

    } catch (const std::exception& e) {
        LOG_ERROR("HTTP request failed: {}", e.what());
    }

    return "";
}

// ============================================================================
// Metadata Management
// ============================================================================

bool SyncManager::updateSyncMetadata(int64_t serverTimestamp) {
    // Update last sync timestamp
    return setLastSyncTimestamp(serverTimestamp);
}

int64_t SyncManager::getLastSyncTimestamp() {
    auto& db = SqliteManager::getInstance();

    auto result = db.query("SELECT value FROM user_preferences WHERE key = 'sync.last_sync_timestamp'");

    if (!result.empty()) {
        try {
            return std::stoll(result[0].getString(0));
        } catch (...) {
            return 0;
        }
    }

    return 0;
}

bool SyncManager::setLastSyncTimestamp(int64_t timestamp) {
    auto& db = SqliteManager::getInstance();

    std::string sql = "INSERT OR REPLACE INTO user_preferences (key, value, value_type) "
                     "VALUES ('sync.last_sync_timestamp', '" +
                     std::to_string(timestamp) + "', 'int')";

    return db.execute(sql);
}

SyncManager::SyncStats SyncManager::getStats() const {
    SyncStats stats;

    stats.lastSyncTimestamp = std::chrono::system_clock::to_time_t(state_.lastSyncTime);

    auto& db = SqliteManager::getInstance();
    auto pending = db.getPapersNeedingSync();
    stats.pendingUploadCount = pending.size();

    stats.avgSyncDuration = state_.lastResult.durationSeconds;

    return stats;
}

// ============================================================================
// Change Tracking
// ============================================================================

bool SyncManager::markForSync(int paperId) {
    auto& db = SqliteManager::getInstance();
    return db.markPaperForSync(paperId);
}

bool SyncManager::markJournalForSync(int journalId) {
    // Similar to markForSync
    return true;
}

std::vector<ChangeEntry> SyncManager::getPendingChanges() {
    auto& db = SqliteManager::getInstance();
    auto papers = db.getPapersNeedingSync();

    std::vector<ChangeEntry> changes;
    for (const auto& paper : papers) {
        ChangeEntry entry;
        entry.tableName = "papers";
        entry.rowId = paper.getId();
        entry.serverId = paper.getServerId();
        entry.version = paper.getSyncVersion();
        // entry.data = serializePaper(paper);
        changes.push_back(entry);
    }

    return changes;
}

} // namespace PaperCrawler
