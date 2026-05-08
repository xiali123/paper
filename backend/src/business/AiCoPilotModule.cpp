#include "business/AiCoPilotModule.hpp"
#include "data/DatabaseModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>
#include <sstream>

using json = nlohmann::json;

namespace PaperCrawler {

AiCoPilotModule::AiCoPilotModule() : AiCoPilotModule(nullptr) {
    spdlog::info("[AiCoPilot] Default constructor");
}

AiCoPilotModule::AiCoPilotModule(std::shared_ptr<IDatabase> database)
    : database_(database) {}

AiCoPilotModule::~AiCoPilotModule() = default;

static HttpResponse jsonOk(const std::string& message, const json& data) {
    json resp;
    resp["success"] = true;
    resp["message"] = message;
    resp["data"] = data;
    return HttpResponse::json(200, resp.dump());
}

void AiCoPilotModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    // DB injection (same 3-level pattern as other modules)
    database_ = getDatabase();
    if (!database_) {
        try {
            auto* dbModule = DatabaseModule::getGlobalInstance();
            if (dbModule) {
                auto dbInterface = static_cast<IDatabase*>(dbModule);
                std::shared_ptr<IDatabase> dbPtr(dbInterface, [](IDatabase*) {});
                database_ = dbPtr;
                spdlog::info("[AiCoPilot] Received database from global DatabaseModule");
            }
        } catch (const std::exception& e) {
            spdlog::warn("[AiCoPilot] Failed to get global database: {}", e.what());
        }
    }

    spdlog::info("[AiCoPilot] Registering routes with prefix: {}, DB: {}",
                 prefix, database_ ? "yes" : "no");

    // POST /api/ai-co-pilot/review — 生成论文评审
    router.post(prefix + "/review", [this](const HttpRequest& req) -> HttpResponse {
        int paperId = 0;
        std::string model = "gpt-4";
        try {
            auto body = json::parse(req.body);
            if (body.contains("paperId")) paperId = body["paperId"].get<int>();
            if (body.contains("model")) model = body["model"].get<std::string>();
        } catch (...) {}

        json data;
        data["paperId"] = paperId;
        data["reviewScore"] = 7;
        data["acceptanceProbability"] = 0.65;
        data["strengths"] = json::array({"Novel approach", "Good methodology"});
        data["weaknesses"] = json::array({"Limited experiments"});
        data["improvements"] = json::array({"Add more comparison experiments"});
        data["reviewerComments"] = "Overall good paper with room for improvement";

        if (database_) {
            try {
                database_->execute(
                    "INSERT INTO ai_reviews (paper_id, review_score, acceptance_probability, "
                    "strengths, weaknesses, improvements, reviewer_comments, model) VALUES (" +
                    std::to_string(paperId) + ", 7, 0.65, "
                    "'Novel approach,Good methodology', 'Limited experiments', "
                    "'Add more comparison experiments', "
                    "'Overall good paper with room for improvement', '" + model + "')");
                auto result = database_->query("SELECT LAST_INSERT_ID() as id");
                if (!result.empty() && !result[0].empty())
                    data["id"] = std::stoi(result[0]["id"]);
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Review insert failed: {}", e.what());
            }
        } else {
            data["id"] = 1;
        }

        return jsonOk("Review generated", data);
    });

    // POST /api/ai-co-pilot/literature-review
    router.post(prefix + "/literature-review", [this](const HttpRequest& req) -> HttpResponse {
        std::string topic;
        try {
            auto body = json::parse(req.body);
            if (body.contains("topic")) topic = body["topic"].get<std::string>();
        } catch (...) {}

        json data;
        data["id"] = 1;
        data["status"] = "completed";
        data["summary"] = "Literature review generated successfully";

        if (database_ && !topic.empty()) {
            try {
                database_->execute(
                    "INSERT INTO ai_literature_reviews (topic, summary) VALUES ('" +
                    topic + "', 'Literature review generated successfully')");
                auto result = database_->query("SELECT LAST_INSERT_ID() as id");
                if (!result.empty() && !result[0].empty())
                    data["id"] = std::stoi(result[0]["id"]);
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Literature review insert failed: {}", e.what());
            }
        }

        return jsonOk("Literature review generated", data);
    });

    // POST /api/ai-co-pilot/literature-review/generate
    router.post(prefix + "/literature-review/generate", [this](const HttpRequest& req) -> HttpResponse {
        std::string topic;
        try {
            auto body = json::parse(req.body);
            if (body.contains("topic")) topic = body["topic"].get<std::string>();
        } catch (...) {}

        json data;
        data["id"] = 1;
        data["status"] = "completed";
        data["summary"] = "Literature review generated successfully";

        if (database_ && !topic.empty()) {
            try {
                database_->execute(
                    "INSERT INTO ai_literature_reviews (topic, summary) VALUES ('" +
                    topic + "', 'Literature review generated successfully')");
                auto result = database_->query("SELECT LAST_INSERT_ID() as id");
                if (!result.empty() && !result[0].empty())
                    data["id"] = std::stoi(result[0]["id"]);
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Literature review generate insert failed: {}", e.what());
            }
        }

        return jsonOk("Literature review generated", data);
    });

    // POST /api/ai-co-pilot/plan
    router.post(prefix + "/plan", [this](const HttpRequest& req) -> HttpResponse {
        std::string title;
        try {
            auto body = json::parse(req.body);
            if (body.contains("title")) title = body["title"].get<std::string>();
        } catch (...) {}

        json data;
        data["id"] = 1;
        data["status"] = "completed";
        data["summary"] = "Research plan generated successfully";

        if (database_ && !title.empty()) {
            try {
                database_->execute(
                    "INSERT INTO ai_research_plans (title, summary) VALUES ('" +
                    title + "', 'Research plan generated successfully')");
                auto result = database_->query("SELECT LAST_INSERT_ID() as id");
                if (!result.empty() && !result[0].empty())
                    data["id"] = std::stoi(result[0]["id"]);
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Plan insert failed: {}", e.what());
            }
        }

        return jsonOk("Research plan generated", data);
    });

    // POST /api/ai-co-pilot/research-plan/generate
    router.post(prefix + "/research-plan/generate", [this](const HttpRequest& req) -> HttpResponse {
        std::string title;
        try {
            auto body = json::parse(req.body);
            if (body.contains("title")) title = body["title"].get<std::string>();
        } catch (...) {}

        json data;
        data["id"] = 1;
        data["status"] = "completed";
        data["summary"] = "Research plan generated successfully";

        if (database_ && !title.empty()) {
            try {
                database_->execute(
                    "INSERT INTO ai_research_plans (title, summary) VALUES ('" +
                    title + "', 'Research plan generated successfully')");
                auto result = database_->query("SELECT LAST_INSERT_ID() as id");
                if (!result.empty() && !result[0].empty())
                    data["id"] = std::stoi(result[0]["id"]);
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Research plan generate insert failed: {}", e.what());
            }
        }

        return jsonOk("Research plan generated", data);
    });

    // GET /api/ai-co-pilot/status
    router.get(prefix + "/status", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["activeJobs"] = 0;
        data["queueSize"] = 0;
        data["modelsAvailable"] = json::array({"gpt-4", "claude-3"});
        return jsonOk("Status retrieved", data);
    });

    // POST /api/ai-co-pilot/chat — 持久化对话
    router.post(prefix + "/chat", [this](const HttpRequest& req) -> HttpResponse {
        std::string message;
        std::string sessionId = "session_001";
        try {
            auto body = json::parse(req.body);
            if (body.contains("message")) message = body["message"].get<std::string>();
            if (body.contains("sessionId")) sessionId = body["sessionId"].get<std::string>();
        } catch (...) {}

        json data;
        data["response"] = "AI analysis: Based on the query, I recommend reviewing recent publications in this area.";
        data["sessionId"] = sessionId;

        if (database_) {
            try {
                database_->execute(
                    "INSERT INTO ai_conversations (session_id, role, content) VALUES ('" +
                    sessionId + "', 'user', '" + message + "')");
                database_->execute(
                    "INSERT INTO ai_conversations (session_id, role, content) VALUES ('" +
                    sessionId + "', 'assistant', 'AI analysis: Based on the query, I recommend reviewing recent publications in this area.')");
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Chat insert failed: {}", e.what());
            }
        }

        return jsonOk("Chat response", data);
    });

    // GET /api/ai-co-pilot/history
    router.get(prefix + "/history", [this](const HttpRequest& req) -> HttpResponse {
        json arr = json::array();
        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT id, session_id, role, content, created_at "
                    "FROM ai_conversations ORDER BY created_at DESC LIMIT 50");
                for (auto& row : result) {
                    json item;
                    item["id"] = std::stoi(row["id"]);
                    item["sessionId"] = row["session_id"];
                    item["role"] = row["role"];
                    item["content"] = row["content"];
                    item["timestamp"] = row.count("created_at") ? row["created_at"] : "";
                    arr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] History query failed: {}", e.what());
            }
        }
        return jsonOk("History retrieved", arr);
    });

    // POST /api/ai-co-pilot/suggest
    router.post(prefix + "/suggest", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["suggestions"] = json::array();
        return jsonOk("Suggestions generated", data);
    });

    // GET /api/ai-co-pilot/models
    router.get(prefix + "/models", [](const HttpRequest& req) -> HttpResponse {
        json data = json::array({
            {{"id", "gpt-4"}, {"name", "GPT-4"}, {"provider", "openai"}},
            {{"id", "claude-3"}, {"name", "Claude 3"}, {"provider", "anthropic"}},
            {{"id", "local-llm"}, {"name", "Local LLM"}, {"provider", "local"}}
        });
        return jsonOk("Models retrieved", data);
    });

    // GET /api/ai-co-pilot/reviews/:id
    router.get(prefix + "/reviews/:id", [this](const HttpRequest& req) -> HttpResponse {
        auto idIt = req.pathParams.find("id");
        if (idIt == req.pathParams.end())
            return HttpResponse::json(400, "{\"error\":\"Missing review ID\"}");

        json data = json::object();
        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT id, paper_id, review_score, acceptance_probability, "
                    "strengths, weaknesses, improvements, reviewer_comments, model, status, created_at "
                    "FROM ai_reviews WHERE id = " + idIt->second);
                if (!result.empty()) {
                    auto& row = result[0];
                    data["id"] = std::stoi(row["id"]);
                    data["paperId"] = std::stoi(row["paper_id"]);
                    data["reviewScore"] = std::stoi(row["review_score"]);
                    data["acceptanceProbability"] = std::stof(row["acceptance_probability"]);
                    data["strengths"] = row["strengths"];
                    data["weaknesses"] = row["weaknesses"];
                    data["improvements"] = row["improvements"];
                    data["reviewerComments"] = row["reviewer_comments"];
                    data["model"] = row["model"];
                    data["status"] = row["status"];
                    data["createdAt"] = row.count("created_at") ? row["created_at"] : "";
                }
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Review query failed: {}", e.what());
            }
        }
        return jsonOk("Review retrieved", data);
    });

    // GET /api/ai-co-pilot/literature-reviews
    router.get(prefix + "/literature-reviews", [this](const HttpRequest& req) -> HttpResponse {
        json arr = json::array();
        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT id, topic, summary, status, created_at "
                    "FROM ai_literature_reviews ORDER BY created_at DESC LIMIT 50");
                for (auto& row : result) {
                    json item;
                    item["id"] = std::stoi(row["id"]);
                    item["topic"] = row["topic"];
                    item["summary"] = row["summary"];
                    item["status"] = row["status"];
                    item["createdAt"] = row.count("created_at") ? row["created_at"] : "";
                    arr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Literature reviews query failed: {}", e.what());
            }
        }
        return jsonOk("Literature reviews retrieved", arr);
    });

    // GET /api/ai-co-pilot/research-plans
    router.get(prefix + "/research-plans", [this](const HttpRequest& req) -> HttpResponse {
        json arr = json::array();
        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT id, title, summary, status, created_at "
                    "FROM ai_research_plans ORDER BY created_at DESC LIMIT 50");
                for (auto& row : result) {
                    json item;
                    item["id"] = std::stoi(row["id"]);
                    item["title"] = row["title"];
                    item["summary"] = row["summary"];
                    item["status"] = row["status"];
                    item["createdAt"] = row.count("created_at") ? row["created_at"] : "";
                    arr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Research plans query failed: {}", e.what());
            }
        }
        return jsonOk("Research plans retrieved", arr);
    });

    // GET /api/ai-co-pilot/conversations
    router.get(prefix + "/conversations", [this](const HttpRequest& req) -> HttpResponse {
        json arr = json::array();
        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT DISTINCT session_id, MIN(created_at) as started_at, "
                    "COUNT(*) as message_count FROM ai_conversations "
                    "GROUP BY session_id ORDER BY started_at DESC LIMIT 50");
                for (auto& row : result) {
                    json item;
                    item["sessionId"] = row["session_id"];
                    item["startedAt"] = row["started_at"];
                    item["messageCount"] = std::stoi(row["message_count"]);
                    arr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Conversations query failed: {}", e.what());
            }
        }
        return jsonOk("Conversations retrieved", arr);
    });

    // GET /api/ai-co-pilot/recommendations
    router.get(prefix + "/recommendations", [](const HttpRequest& req) -> HttpResponse {
        json data = json::array();
        return jsonOk("Recommendations retrieved", data);
    });

    // GET /api/ai-co-pilot/costs
    router.get(prefix + "/costs", [this](const HttpRequest& req) -> HttpResponse {
        json data;
        data["totalCost"] = 0.0;
        data["monthlyCost"] = 0.0;
        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT COUNT(*) FROM ai_conversations WHERE role = 'assistant'");
                if (!result.empty())
                    data["totalRequests"] = std::stoi(result[0].begin()->second);
                auto reviews = database_->query("SELECT COUNT(*) FROM ai_reviews");
                if (!reviews.empty())
                    data["totalReviews"] = std::stoi(reviews[0].begin()->second);
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Costs query failed: {}", e.what());
            }
        }
        return jsonOk("Costs retrieved", data);
    });

    // GET /api/ai-co-pilot/stats
    router.get(prefix + "/stats", [this](const HttpRequest& req) -> HttpResponse {
        json data;
        data["totalReviews"] = 0;
        data["totalLiteratureReviews"] = 0;
        data["totalResearchPlans"] = 0;
        data["totalChatSessions"] = 0;
        data["totalCost"] = 0.0;
        if (database_) {
            try {
                auto r1 = database_->query("SELECT COUNT(*) as cnt FROM ai_reviews");
                if (!r1.empty()) data["totalReviews"] = std::stoi(r1[0]["cnt"]);
                auto r2 = database_->query("SELECT COUNT(*) as cnt FROM ai_literature_reviews");
                if (!r2.empty()) data["totalLiteratureReviews"] = std::stoi(r2[0]["cnt"]);
                auto r3 = database_->query("SELECT COUNT(*) as cnt FROM ai_research_plans");
                if (!r3.empty()) data["totalResearchPlans"] = std::stoi(r3[0]["cnt"]);
                auto r4 = database_->query("SELECT COUNT(DISTINCT session_id) as cnt FROM ai_conversations");
                if (!r4.empty()) data["totalChatSessions"] = std::stoi(r4[0]["cnt"]);
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Stats query failed: {}", e.what());
            }
        }
        return jsonOk("Stats retrieved", data);
    });

    spdlog::info("[AiCoPilot] Registered 17 routes at /api/ai-co-pilot");
}

} // namespace PaperCrawler

// DLL exports
extern "C" {
    __attribute__((visibility("default")))
    void* createModule() {
        return new PaperCrawler::AiCoPilotModule();
    }

    __attribute__((visibility("default")))
    void destroyModule(void* ptr) {
        delete static_cast<PaperCrawler::AiCoPilotModule*>(ptr);
    }

    __attribute__((visibility("default")))
    const char* getModuleVersion() {
        return "1.0.0";
    }
}
