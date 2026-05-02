#include "core/GrpcBridge.hpp"
#include "data/IDatabase.hpp"
#include <spdlog/spdlog.h>
#include "../../core/external/nlohmann/json.hpp"

namespace PaperCrawler {

using json = nlohmann::json;

void registerGrpcServices(std::shared_ptr<IDatabase> database) {
    auto& bridge = GrpcBridge::instance();

    // === SearchService ===
    bridge.registerService("SearchService");

    bridge.registerMethod("SearchService", "Search",
        [database](const std::string& request) -> std::string {
            json resp;
            if (!database) {
                resp["error"] = "Database not available";
                return resp.dump();
            }
            try {
                auto req = json::parse(request);
                std::string query = req.value("query", "");
                int limit = req.value("limit", 20);

                std::string sql = "SELECT id, title, authors, year, abstract FROM papers "
                                  "WHERE MATCH(title, abstract) AGAINST(?) LIMIT ?";
                // Simplified - return mock for now
                resp["results"] = json::array();
                resp["total"] = 0;
                resp["query"] = query;
            } catch (const std::exception& e) {
                resp["error"] = e.what();
            }
            return resp.dump();
        });

    bridge.registerMethod("SearchService", "GetPaper",
        [database](const std::string& request) -> std::string {
            json resp;
            if (!database) { resp["error"] = "Database not available"; return resp.dump(); }
            try {
                auto req = json::parse(request);
                int id = req.value("id", 0);
                // Would query papers table
                resp["id"] = id;
                resp["found"] = false;
            } catch (const std::exception& e) {
                resp["error"] = e.what();
            }
            return resp.dump();
        });

    bridge.registerMethod("SearchService", "SuggestKeywords",
        [](const std::string& request) -> std::string {
            json resp;
            resp["keywords"] = json::array();
            return resp.dump();
        });

    bridge.registerMethod("SearchService", "GetPapersBatch",
        [database](const std::string& request) -> std::string {
            json resp;
            resp["papers"] = json::array();
            return resp.dump();
        });

    // === HealthService ===
    bridge.registerService("HealthService");

    bridge.registerMethod("HealthService", "Check",
        [](const std::string&) -> std::string {
            json resp;
            resp["status"] = "SERVING";
            return resp.dump();
        });

    bridge.registerMethod("HealthService", "Watch",
        [](const std::string&) -> std::string {
            json resp;
            resp["status"] = "SERVING";
            return resp.dump();
        });

    // === AnalyticsService ===
    bridge.registerService("AnalyticsService");

    bridge.registerMethod("AnalyticsService", "GetStatistics",
        [database](const std::string& request) -> std::string {
            json resp;
            resp["totalPapers"] = 0;
            resp["totalUsers"] = 0;
            return resp.dump();
        });

    bridge.registerMethod("AnalyticsService", "GetTrends",
        [](const std::string& request) -> std::string {
            json resp;
            resp["trends"] = json::array();
            return resp.dump();
        });

    bridge.registerMethod("AnalyticsService", "GetJournalRankings",
        [](const std::string& request) -> std::string {
            json resp;
            resp["rankings"] = json::array();
            return resp.dump();
        });

    bridge.registerMethod("AnalyticsService", "GetPublicationTrends",
        [](const std::string& request) -> std::string {
            json resp;
            resp["trends"] = json::array();
            return resp.dump();
        });

    // === SyncService ===
    bridge.registerService("SyncService");
    bridge.registerMethod("SyncService", "SyncChanges",
        [](const std::string&) -> std::string {
            json resp;
            resp["changes"] = json::array();
            return resp.dump();
        });
    bridge.registerMethod("SyncService", "GetSyncStatus",
        [](const std::string&) -> std::string {
            json resp;
            resp["lastSync"] = "";
            resp["pending"] = 0;
            return resp.dump();
        });

    // === AuthService ===
    bridge.registerService("AuthService");
    bridge.registerMethod("AuthService", "Login",
        [](const std::string&) -> std::string {
            json resp;
            resp["token"] = "";
            resp["error"] = "Not implemented";
            return resp.dump();
        });
    bridge.registerMethod("AuthService", "ValidateToken",
        [](const std::string&) -> std::string {
            json resp;
            resp["valid"] = false;
            return resp.dump();
        });

    spdlog::info("[GrpcBridge] All {} services registered", bridge.listMethods().size());
}

} // namespace PaperCrawler
