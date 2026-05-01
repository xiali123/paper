#include "features/ai/VectorStore.hpp"
#include "features/ai/EmbeddingGenerator.hpp"
#include "data/IDatabase.hpp"
#include <spdlog/spdlog.h>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iomanip>

namespace PaperCrawler {

VectorStore::VectorStore() = default;

void VectorStore::setDatabase(std::shared_ptr<IDatabase> db) {
    database_ = std::move(db);
}

bool VectorStore::initializeTable() {
    if (!database_) {
        spdlog::warn("[VectorStore] No database available for table initialization");
        return false;
    }

    const std::string createSQL = R"(
        CREATE TABLE IF NOT EXISTS paper_embeddings (
            id TEXT PRIMARY KEY,
            content TEXT NOT NULL,
            vector TEXT NOT NULL,
            metadata TEXT DEFAULT '{}',
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";

    try {
        bool result = database_->createTableIfNotExists("paper_embeddings", createSQL);
        if (result) {
            spdlog::info("[VectorStore] Table 'paper_embeddings' initialized successfully");
        } else {
            // Fallback: try direct execute
            result = database_->execute(createSQL);
        }
        return result;
    } catch (const std::exception& e) {
        spdlog::error("[VectorStore] Failed to initialize table: {}", e.what());
        return false;
    }
}

bool VectorStore::storeEmbedding(const Embedding& embedding) {
    // Update L1 cache
    cache_[embedding.id] = embedding;

    if (!database_) {
        return true; // Cache-only mode
    }

    try {
        std::string vectorJson = vectorToJson(embedding.vector);

        // Serialize metadata to JSON
        std::string metadataJson = "{";
        bool first = true;
        for (const auto& [key, value] : embedding.metadata) {
            if (!first) metadataJson += ",";
            first = false;
            // Escape quotes in key and value
            std::string escapedKey = key;
            std::string escapedValue = value;
            // Simple JSON escaping
            metadataJson += "\"" + escapedKey + "\":\"" + escapedValue + "\"";
        }
        metadataJson += "}";

        // Escape content for SQL
        std::string escapedContent = database_->escapeString(embedding.content);
        std::string escapedVector = database_->escapeString(vectorJson);
        std::string escapedMetadata = database_->escapeString(metadataJson);
        std::string escapedId = database_->escapeString(embedding.id);

        std::ostringstream sql;
        sql << "INSERT OR REPLACE INTO paper_embeddings (id, content, vector, metadata, updated_at) VALUES ('"
            << escapedId << "', '"
            << escapedContent << "', '"
            << escapedVector << "', '"
            << escapedMetadata << "', CURRENT_TIMESTAMP)";

        return database_->execute(sql.str());
    } catch (const std::exception& e) {
        spdlog::error("[VectorStore] Failed to store embedding {}: {}", embedding.id, e.what());
        return false;
    }
}

bool VectorStore::storeEmbeddings(const std::vector<Embedding>& embeddings) {
    int successCount = 0;
    for (const auto& emb : embeddings) {
        if (storeEmbedding(emb)) {
            successCount++;
        }
    }
    spdlog::info("[VectorStore] Stored {}/{} embeddings", successCount, embeddings.size());
    return successCount > 0;
}

std::vector<SearchResult> VectorStore::search(
    const std::vector<float>& queryVector,
    int topK,
    float minScore) {

    if (queryVector.empty()) {
        spdlog::warn("[VectorStore] Empty query vector");
        return {};
    }

    // Load from DB into cache if cache is empty and DB is available
    if (cache_.empty() && database_) {
        loadAllToCache();
    }

    return searchInMemory(queryVector, topK, minScore);
}

std::vector<SearchResult> VectorStore::searchInMemory(
    const std::vector<float>& queryVector,
    int topK,
    float minScore) {

    std::vector<SearchResult> results;

    for (const auto& [id, embedding] : cache_) {
        if (embedding.vector.empty()) continue;
        if (embedding.vector.size() != queryVector.size()) continue;

        float score = cosineSimilarity(queryVector, embedding.vector);
        if (score >= minScore) {
            SearchResult result;
            result.id = embedding.id;
            result.score = score;
            result.content = embedding.content;
            result.metadata = embedding.metadata;
            results.push_back(std::move(result));
        }
    }

    // Sort by score descending
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.score > b.score;
        });

    // Return top-K
    if (static_cast<int>(results.size()) > topK) {
        results.resize(topK);
    }

    return results;
}

std::optional<Embedding> VectorStore::getEmbedding(const std::string& id) {
    // Check L1 cache first
    auto it = cache_.find(id);
    if (it != cache_.end()) {
        return it->second;
    }

    // Query from database
    if (!database_) {
        return std::nullopt;
    }

    try {
        std::string escapedId = database_->escapeString(id);
        std::string sql = "SELECT id, content, vector, metadata FROM paper_embeddings WHERE id = '" + escapedId + "'";
        auto rows = database_->query(sql);

        if (rows.empty()) {
            return std::nullopt;
        }

        const auto& row = rows[0];
        Embedding emb;
        emb.id = row.at("id");
        emb.content = row.at("content");
        emb.vector = jsonToVector(row.at("vector"));

        // Simple metadata parse (key:value pairs)
        // For production, use a proper JSON parser
        emb.metadata["source"] = "database";

        // Cache it
        cache_[emb.id] = emb;
        return emb;
    } catch (const std::exception& e) {
        spdlog::error("[VectorStore] Failed to get embedding {}: {}", id, e.what());
        return std::nullopt;
    }
}

bool VectorStore::deleteEmbedding(const std::string& id) {
    // Remove from cache
    cache_.erase(id);

    if (!database_) {
        return true;
    }

    try {
        std::string escapedId = database_->escapeString(id);
        std::string sql = "DELETE FROM paper_embeddings WHERE id = '" + escapedId + "'";
        return database_->execute(sql);
    } catch (const std::exception& e) {
        spdlog::error("[VectorStore] Failed to delete embedding {}: {}", id, e.what());
        return false;
    }
}

int VectorStore::indexPapersFromDatabase(std::shared_ptr<IDatabase> database, int limit) {
    if (!database) {
        spdlog::warn("[VectorStore] No database provided for indexing");
        return 0;
    }

    try {
        std::string sql = "SELECT id, title, abstract FROM papers ORDER BY id DESC LIMIT " + std::to_string(limit);
        auto rows = database->query(sql);

        if (rows.empty()) {
            spdlog::info("[VectorStore] No papers found for indexing");
            return 0;
        }

        spdlog::info("[VectorStore] Generating embeddings for {} papers...", rows.size());

        int indexedCount = 0;
        for (const auto& row : rows) {
            std::string paperId = row.count("id") ? row.at("id") : "";
            std::string title = row.count("title") ? row.at("title") : "";
            std::string abstract = row.count("abstract") ? row.at("abstract") : "";

            if (paperId.empty()) continue;

            // Combine title and abstract for embedding content
            std::string content = title;
            if (!abstract.empty()) {
                content += " " + abstract;
            }

            if (content.empty()) continue;

            // Generate embedding using local TF-hash method
            std::vector<float> vec = EmbeddingGenerator::generateLocalEmbedding(content, 384);

            Embedding embedding;
            embedding.id = paperId;
            embedding.content = content;
            embedding.vector = std::move(vec);
            embedding.metadata["title"] = title;
            embedding.metadata["type"] = "paper";

            if (storeEmbedding(embedding)) {
                indexedCount++;
            }
        }

        spdlog::info("[VectorStore] Indexed {}/{} papers successfully", indexedCount, rows.size());
        return indexedCount;
    } catch (const std::exception& e) {
        spdlog::error("[VectorStore] Failed to index papers: {}", e.what());
        return 0;
    }
}

void VectorStore::loadAllToCache() {
    if (!database_) return;

    try {
        auto rows = database_->query(
            "SELECT id, content, vector, metadata FROM paper_embeddings");

        for (const auto& row : rows) {
            Embedding emb;
            emb.id = row.at("id");
            emb.content = row.at("content");
            emb.vector = jsonToVector(row.at("vector"));
            emb.metadata["source"] = "database";
            cache_[emb.id] = std::move(emb);
        }

        spdlog::info("[VectorStore] Loaded {} embeddings into cache", cache_.size());
    } catch (const std::exception& e) {
        spdlog::error("[VectorStore] Failed to load embeddings to cache: {}", e.what());
    }
}

// --- Static utility methods ---

float VectorStore::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) {
        return 0.0f;
    }

    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;

    for (size_t i = 0; i < a.size(); ++i) {
        dotProduct += a[i] * b[i];
        normA += a[i] * a[i];
        normB += b[i] * b[i];
    }

    float denominator = std::sqrt(normA) * std::sqrt(normB);
    if (denominator < 1e-8f) {
        return 0.0f;
    }

    return dotProduct / denominator;
}

std::string VectorStore::vectorToJson(const std::vector<float>& vec) {
    if (vec.empty()) return "[]";

    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) oss << ",";
        // Use fixed precision for consistent serialization
        oss << std::fixed << std::setprecision(8) << vec[i];
    }
    oss << "]";
    return oss.str();
}

std::vector<float> VectorStore::jsonToVector(const std::string& json) {
    std::vector<float> result;

    // Simple JSON array parser: [0.1,0.2,...]
    // Strip whitespace and brackets
    std::string cleaned;
    for (char c : json) {
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            cleaned += c;
        }
    }

    if (cleaned.size() < 2 || cleaned.front() != '[' || cleaned.back() != ']') {
        return result;
    }

    // Remove brackets
    cleaned = cleaned.substr(1, cleaned.size() - 2);

    // Split by comma
    std::istringstream iss(cleaned);
    std::string token;
    while (std::getline(iss, token, ',')) {
        try {
            float val = std::stof(token);
            result.push_back(val);
        } catch (...) {
            // Skip malformed values
        }
    }

    return result;
}

} // namespace PaperCrawler
