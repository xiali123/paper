#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace PaperCrawler {

// Meilisearch document representation
struct MeiliDocument {
    std::string id;
    std::string title;
    std::string abstract;
    std::string authors;
    std::string keywords;
    std::string journal;
    int year{0};
    int citationCount{0};

    std::string toJson() const;
};

// Meilisearch search result hit
struct MeiliSearchResult {
    std::string id;
    std::string title;
    std::string abstract;
    std::string authors;
    float relevanceScore{0.0f};
    std::map<std::string, std::string> formatted; // _formatted fields
};

// Meilisearch search response
struct MeiliSearchResponse {
    std::vector<MeiliSearchResult> hits;
    int estimatedTotalHits{0};
    int offset{0};
    int limit{20};
    int processingTimeMs{0};
    std::string query;
};

// Meilisearch HTTP client
class MeilisearchClient {
public:
    MeilisearchClient();
    explicit MeilisearchClient(const std::string& host, const std::string& apiKey = "");
    ~MeilisearchClient() = default;

    // Index management
    bool createIndex(const std::string& uid, const std::string& primaryKey = "id");
    bool deleteIndex(const std::string& uid);
    bool updateIndexSettings(const std::string& uid, const std::string& settingsJson);

    // Document operations
    bool addDocuments(const std::string& uid, const std::vector<MeiliDocument>& docs);
    bool updateDocuments(const std::string& uid, const std::vector<MeiliDocument>& docs);
    bool deleteDocument(const std::string& uid, const std::string& docId);
    bool deleteAllDocuments(const std::string& uid);
    int64_t countDocuments(const std::string& uid);

    // Search
    MeiliSearchResponse search(const std::string& uid, const std::string& query,
                                int limit = 20, int offset = 0,
                                const std::vector<std::string>& attributesToHighlight = {"title", "abstract"});

    // Sync from database (batch import)
    int syncPapersFromDatabase(std::shared_ptr<class IDatabase> database,
                                const std::string& indexUid = "papers",
                                int batchSize = 1000);

    // Health check
    bool isHealthy();
    std::string getVersion();

    // Configuration
    void setHost(const std::string& host);
    void setApiKey(const std::string& key);
    void setTimeout(int seconds);

    // HTTP request helpers (public for delegated use by SearchApiModule)
    std::string httpRequest(const std::string& method, const std::string& path,
                             const std::string& body = "");
    std::string get(const std::string& path);
    std::string post(const std::string& path, const std::string& body);
    std::string put(const std::string& path, const std::string& body);
    std::string patch(const std::string& path, const std::string& body);
    std::string del(const std::string& path);

private:
    std::string host_{"http://localhost:7700"};
    std::string apiKey_;
    int timeout_{10};

    // Parse search response JSON
    MeiliSearchResponse parseSearchResponse(const std::string& json);
};

} // namespace PaperCrawler
