#include "features/search/MeilisearchClient.hpp"
#include "data/IDatabase.hpp"
#include "data/PreparedStatement.hpp"

#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <array>
#include <fstream>
#include <spdlog/spdlog.h>

// nlohmann/json -- the project includes it via the core/external path
#include <json.hpp>

namespace PaperCrawler {

// ============================================================================
// MeiliDocument
// ============================================================================

std::string MeiliDocument::toJson() const {
    nlohmann::json j;
    j["id"]            = id;
    j["title"]         = title;
    j["abstract"]      = abstract;
    j["authors"]       = authors;
    j["keywords"]      = keywords;
    j["journal"]       = journal;
    j["year"]          = year;
    j["citationCount"] = citationCount;
    return j.dump();
}

// ============================================================================
// MeilisearchClient construction
// ============================================================================

MeilisearchClient::MeilisearchClient() = default;

MeilisearchClient::MeilisearchClient(const std::string& host, const std::string& apiKey)
    : host_(host), apiKey_(apiKey) {}

// ============================================================================
// Configuration setters
// ============================================================================

void MeilisearchClient::setHost(const std::string& host) { host_ = host; }
void MeilisearchClient::setApiKey(const std::string& key) { apiKey_ = key; }
void MeilisearchClient::setTimeout(int seconds) { timeout_ = seconds; }

// ============================================================================
// Low-level HTTP helpers (curl via popen -- same pattern as EmailService)
// ============================================================================

std::string MeilisearchClient::httpRequest(const std::string& method,
                                            const std::string& path,
                                            const std::string& body) {
    // Build the curl command
    std::ostringstream cmd;
    cmd << "curl -s -S"
        << " --max-time " << timeout_
        << " -X " << method;

    // Meilisearch API key header
    if (!apiKey_.empty()) {
        cmd << " -H 'Authorization: Bearer " << apiKey_ << "'";
    }

    cmd << " -H 'Content-Type: application/json'";

    // Request body via --data-binary (escaped for shell)
    if (!body.empty()) {
        // Write body to a temp file to avoid shell escaping issues
        std::string tmpFile = "/tmp/meili_body_" +
            std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()) +
            ".json";

        {
            std::ofstream ofs(tmpFile);
            if (ofs.is_open()) {
                ofs << body;
            }
        }

        cmd << " -d @" << tmpFile;

        // Execute and capture output
        std::string fullCmd = cmd.str() + " '" + host_ + path + "' 2>&1";

        spdlog::debug("[MeilisearchClient] HTTP {} {}", method, path);

        std::array<char, 4096> buffer{};
        std::string output;

        FILE* pipe = popen(fullCmd.c_str(), "r");
        if (!pipe) {
            std::remove(tmpFile.c_str());
            spdlog::error("[MeilisearchClient] popen() failed for {} {}", method, path);
            return "";
        }

        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            output += buffer.data();
        }

        int status = pclose(pipe);
        std::remove(tmpFile.c_str());

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            spdlog::warn("[MeilisearchClient] curl exited with code {} for {} {}",
                         WEXITSTATUS(status), method, path);
        }

        return output;
    }

    // No body -- simpler command
    std::string fullCmd = cmd.str() + " '" + host_ + path + "' 2>&1";

    spdlog::debug("[MeilisearchClient] HTTP {} {}", method, path);

    std::array<char, 4096> buffer{};
    std::string output;

    FILE* pipe = popen(fullCmd.c_str(), "r");
    if (!pipe) {
        spdlog::error("[MeilisearchClient] popen() failed for {} {}", method, path);
        return "";
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    int status = pclose(pipe);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        spdlog::warn("[MeilisearchClient] curl exited with code {} for {} {}",
                     WEXITSTATUS(status), method, path);
    }

    return output;
}

std::string MeilisearchClient::get(const std::string& path) {
    return httpRequest("GET", path);
}

std::string MeilisearchClient::post(const std::string& path, const std::string& body) {
    return httpRequest("POST", path, body);
}

std::string MeilisearchClient::put(const std::string& path, const std::string& body) {
    return httpRequest("PUT", path, body);
}

std::string MeilisearchClient::patch(const std::string& path, const std::string& body) {
    return httpRequest("PATCH", path, body);
}

std::string MeilisearchClient::del(const std::string& path) {
    return httpRequest("DELETE", path);
}

// ============================================================================
// Index management
// ============================================================================

bool MeilisearchClient::createIndex(const std::string& uid, const std::string& primaryKey) {
    nlohmann::json body;
    body["uid"] = uid;
    body["primaryKey"] = primaryKey;

    std::string resp = post("/indexes", body.dump());

    try {
        auto j = nlohmann::json::parse(resp);
        if (j.contains("status") && (j["status"] == "created" || j["status"] == "processed")) {
            spdlog::info("[MeilisearchClient] Index '{}' created successfully", uid);
            return true;
        }
        // Index might already exist -- that is fine
        if (j.contains("code") && j["code"] == "index_already_exists") {
            spdlog::info("[MeilisearchClient] Index '{}' already exists", uid);
            return true;
        }
        spdlog::warn("[MeilisearchClient] createIndex unexpected response: {}", resp);
    } catch (const std::exception& e) {
        spdlog::error("[MeilisearchClient] createIndex parse error: {}", e.what());
    }
    return false;
}

bool MeilisearchClient::deleteIndex(const std::string& uid) {
    std::string resp = del("/indexes/" + uid);
    try {
        auto j = nlohmann::json::parse(resp);
        return j.contains("status") && (j["status"] == "deleted" || j["status"] == "processed");
    } catch (...) {}
    return false;
}

bool MeilisearchClient::updateIndexSettings(const std::string& uid, const std::string& settingsJson) {
    std::string resp = patch("/indexes/" + uid + "/settings", settingsJson);
    try {
        auto j = nlohmann::json::parse(resp);
        return j.contains("status");
    } catch (...) {}
    return false;
}

// ============================================================================
// Document operations
// ============================================================================

bool MeilisearchClient::addDocuments(const std::string& uid,
                                      const std::vector<MeiliDocument>& docs) {
    if (docs.empty()) return true;

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& doc : docs) {
        arr.push_back(nlohmann::json::parse(doc.toJson()));
    }

    std::string resp = post("/indexes/" + uid + "/documents", arr.dump());

    try {
        auto j = nlohmann::json::parse(resp);
        if (j.contains("status")) {
            spdlog::info("[MeilisearchClient] Added {} documents to index '{}' (task: {})",
                         docs.size(), uid, j.value("taskUid", -1));
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("[MeilisearchClient] addDocuments parse error: {}", e.what());
    }
    return false;
}

bool MeilisearchClient::updateDocuments(const std::string& uid,
                                         const std::vector<MeiliDocument>& docs) {
    if (docs.empty()) return true;

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& doc : docs) {
        arr.push_back(nlohmann::json::parse(doc.toJson()));
    }

    std::string resp = put("/indexes/" + uid + "/documents", arr.dump());

    try {
        auto j = nlohmann::json::parse(resp);
        return j.contains("status");
    } catch (...) {}
    return false;
}

bool MeilisearchClient::deleteDocument(const std::string& uid, const std::string& docId) {
    std::string resp = del("/indexes/" + uid + "/documents/" + docId);
    try {
        auto j = nlohmann::json::parse(resp);
        return j.contains("status");
    } catch (...) {}
    return false;
}

bool MeilisearchClient::deleteAllDocuments(const std::string& uid) {
    std::string resp = del("/indexes/" + uid + "/documents");
    try {
        auto j = nlohmann::json::parse(resp);
        return j.contains("status");
    } catch (...) {}
    return false;
}

int64_t MeilisearchClient::countDocuments(const std::string& uid) {
    std::string resp = get("/indexes/" + uid + "/stats");
    try {
        auto j = nlohmann::json::parse(resp);
        if (j.contains("numberOfDocuments")) {
            return j["numberOfDocuments"].get<int64_t>();
        }
    } catch (...) {}
    return -1;
}

// ============================================================================
// Search
// ============================================================================

MeiliSearchResponse MeilisearchClient::search(const std::string& uid,
                                               const std::string& query,
                                               int limit, int offset,
                                               const std::vector<std::string>& attributesToHighlight) {
    nlohmann::json body;
    body["q"]       = query;
    body["limit"]   = limit;
    body["offset"]  = offset;

    if (!attributesToHighlight.empty()) {
        body["attributesToHighlight"] = attributesToHighlight;
    }

    std::string resp = post("/indexes/" + uid + "/search", body.dump());

    return parseSearchResponse(resp);
}

// ============================================================================
// Parse search response
// ============================================================================

MeiliSearchResponse MeilisearchClient::parseSearchResponse(const std::string& json) {
    MeiliSearchResponse response;

    if (json.empty()) {
        spdlog::warn("[MeilisearchClient] Empty response from Meilisearch");
        return response;
    }

    try {
        auto j = nlohmann::json::parse(json);

        // Check for error responses
        if (j.contains("code") && j.contains("message")) {
            spdlog::error("[MeilisearchClient] Meilisearch error: {} - {}",
                          j["code"].get<std::string>(),
                          j["message"].get<std::string>());
            return response;
        }

        response.estimatedTotalHits = j.value("estimatedTotalHits", 0);
        response.offset             = j.value("offset", 0);
        response.limit              = j.value("limit", 20);
        response.processingTimeMs   = j.value("processingTimeMs", 0);
        response.query              = j.value("q", "");

        if (j.contains("hits") && j["hits"].is_array()) {
            for (const auto& hit : j["hits"]) {
                MeiliSearchResult result;
                result.id     = std::to_string(hit.value("id", 0));
                result.title  = hit.value("title", std::string{});
                result.abstract = hit.value("abstract", std::string{});
                result.authors  = hit.value("authors", std::string{});

                // Meilisearch relevance is 0..1 range for _rankingScore (v1.3+)
                if (hit.contains("_rankingScore")) {
                    result.relevanceScore = hit["_rankingScore"].get<float>();
                }

                // Extract _formatted fields for highlighting
                if (hit.contains("_formatted") && hit["_formatted"].is_object()) {
                    for (auto it = hit["_formatted"].begin(); it != hit["_formatted"].end(); ++it) {
                        if (it.value().is_string()) {
                            result.formatted[it.key()] = it.value().get<std::string>();
                        }
                    }
                }

                response.hits.push_back(std::move(result));
            }
        }
    } catch (const std::exception& e) {
        spdlog::error("[MeilisearchClient] Failed to parse search response: {}", e.what());
    }

    return response;
}

// ============================================================================
// Sync papers from database
// ============================================================================

int MeilisearchClient::syncPapersFromDatabase(std::shared_ptr<IDatabase> database,
                                               const std::string& indexUid,
                                               int batchSize) {
    if (!database) {
        spdlog::error("[MeilisearchClient] Cannot sync: database is null");
        return 0;
    }

    // Ensure index exists
    if (!createIndex(indexUid)) {
        spdlog::error("[MeilisearchClient] Failed to create index '{}'", indexUid);
        return 0;
    }

    // Configure searchable attributes and ranking rules
    nlohmann::json settings;
    settings["searchableAttributes"] = {"title", "abstract", "authors", "keywords"};
    settings["filterableAttributes"] = {"year", "citationCount", "journal"};
    settings["sortableAttributes"]   = {"year", "citationCount"};
    settings["rankingRules"] = {"words", "typo", "proximity", "attribute", "sort", "exactness"};
    updateIndexSettings(indexUid, settings.dump());

    int totalSynced = 0;
    int offset = 0;
    bool hasMore = true;

    while (hasMore) {
        try {
            PreparedStatement stmt(database,
                "SELECT id, title, abstract, authors, keywords, journal, year, citation_count "
                "FROM papers ORDER BY id LIMIT ? OFFSET ?");
            stmt.bind(0, batchSize).bind(1, offset);

            auto rows = stmt.query();

            if (rows.empty()) {
                hasMore = false;
                break;
            }

            // Convert rows to MeiliDocument
            std::vector<MeiliDocument> docs;
            docs.reserve(rows.size());

            for (const auto& row : rows) {
                MeiliDocument doc;
                doc.id            = row.at("id");
                doc.title         = row.count("title")   ? row.at("title")   : "";
                doc.abstract      = row.count("abstract") ? row.at("abstract") : "";
                doc.authors       = row.count("authors")  ? row.at("authors")  : "";
                doc.keywords      = row.count("keywords") ? row.at("keywords") : "";
                doc.journal       = row.count("journal")  ? row.at("journal")  : "";
                doc.year          = row.count("year") && !row.at("year").empty()
                                        ? std::stoi(row.at("year")) : 0;
                doc.citationCount = row.count("citation_count") && !row.at("citation_count").empty()
                                        ? std::stoi(row.at("citation_count")) : 0;
                docs.push_back(std::move(doc));
            }

            // Batch add to Meilisearch
            if (addDocuments(indexUid, docs)) {
                totalSynced += static_cast<int>(docs.size());
                spdlog::info("[MeilisearchClient] Synced {} papers (total: {})",
                             docs.size(), totalSynced);
            } else {
                spdlog::warn("[MeilisearchClient] Failed to sync batch at offset {}", offset);
            }

            offset += batchSize;

            // If we got fewer rows than batchSize, we have reached the end
            if (static_cast<int>(rows.size()) < batchSize) {
                hasMore = false;
            }

        } catch (const std::exception& e) {
            spdlog::error("[MeilisearchClient] Sync error at offset {}: {}", offset, e.what());
            hasMore = false;
        }
    }

    spdlog::info("[MeilisearchClient] Sync complete: {} papers indexed", totalSynced);
    return totalSynced;
}

// ============================================================================
// Health check
// ============================================================================

bool MeilisearchClient::isHealthy() {
    std::string resp = get("/health");
    if (resp.empty()) return false;

    try {
        auto j = nlohmann::json::parse(resp);
        return j.contains("status") && j["status"] == "available";
    } catch (...) {}
    return false;
}

std::string MeilisearchClient::getVersion() {
    std::string resp = get("/version");
    try {
        auto j = nlohmann::json::parse(resp);
        return j.value("pkgVersion", std::string{"unknown"});
    } catch (...) {}
    return "unknown";
}

} // namespace PaperCrawler
