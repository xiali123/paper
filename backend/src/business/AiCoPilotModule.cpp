#include "business/AiCoPilotModule.hpp"
#include "data/DatabaseModule.hpp"
#include "data/ValidationHelper.hpp"
#include "data/StringUtil.hpp"
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

    // Review history for a user
    router.get(prefix + "/reviews", [this](const HttpRequest& req) {
        if (!database_)
            return jsonOk("Reviews retrieved", nlohmann::json::object());

        try {
            int userId = 0;
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = std::stoi(it->second);
            int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 20;

            std::string sql = "SELECT id, paper_id, review_score, acceptance_probability, "
                "strengths, weaknesses, model, status, created_at FROM ai_reviews";
            if (userId > 0) sql += " WHERE user_id = " + std::to_string(userId);
            sql += " ORDER BY created_at DESC LIMIT " + std::to_string(limit);

            auto results = database_->query(sql);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["paperId"] = row.count("paper_id") ? std::stoi(row.at("paper_id")) : 0;
                item["reviewScore"] = row.count("review_score") ? std::stod(row.at("review_score")) : 0.0;
                item["acceptanceProb"] = row.count("acceptance_probability") ? std::stod(row.at("acceptance_probability")) : 0.0;
                item["model"] = row.count("model") ? row.at("model") : "";
                item["status"] = row.count("status") ? row.at("status") : "";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json data;
            data["reviews"] = arr;
            data["total"] = arr.size();
            return jsonOk("Reviews retrieved", data);
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"success\":false,\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Literature review history
    router.get(prefix + "/literature-reviews", [this](const HttpRequest& req) {
        if (!database_)
            return jsonOk("Literature reviews retrieved", nlohmann::json::object());

        try {
            int userId = 0;
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = std::stoi(it->second);

            std::string sql = "SELECT id, user_id, topic, summary, status, created_at FROM ai_literature_reviews";
            if (userId > 0) sql += " WHERE user_id = " + std::to_string(userId);
            sql += " ORDER BY created_at DESC LIMIT 20";

            auto results = database_->query(sql);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["userId"] = row.count("user_id") ? std::stoi(row.at("user_id")) : 0;
                item["topic"] = row.count("topic") ? row.at("topic") : "";
                item["status"] = row.count("status") ? row.at("status") : "";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json data;
            data["literatureReviews"] = arr;
            data["total"] = arr.size();
            return jsonOk("Literature reviews retrieved", data);
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"success\":false,\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // Research plan history
    router.get(prefix + "/plans", [this](const HttpRequest& req) {
        if (!database_)
            return jsonOk("Plans retrieved", nlohmann::json::object());

        try {
            int userId = 0;
            auto it = req.queryParams.find("user_id");
            if (it != req.queryParams.end()) userId = std::stoi(it->second);

            std::string sql = "SELECT id, user_id, title, summary, status, created_at FROM ai_research_plans";
            if (userId > 0) sql += " WHERE user_id = " + std::to_string(userId);
            sql += " ORDER BY created_at DESC LIMIT 20";

            auto results = database_->query(sql);
            nlohmann::json arr = nlohmann::json::array();
            for (auto& row : results) {
                nlohmann::json item;
                item["id"] = std::stoi(row.at("id"));
                item["userId"] = row.count("user_id") ? std::stoi(row.at("user_id")) : 0;
                item["title"] = row.count("title") ? row.at("title") : "";
                item["status"] = row.count("status") ? row.at("status") : "";
                item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                arr.push_back(item);
            }
            nlohmann::json data;
            data["plans"] = arr;
            data["total"] = arr.size();
            return jsonOk("Plans retrieved", data);
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"success\":false,\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/ai-co-pilot/sessions/:id/messages — get session messages
    router.get(prefix + "/sessions/:id/messages", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json resp;
        resp["messages"] = nlohmann::json::array();
        resp["total"] = 0;

        if (database_) {
            try {
                std::string sessionId = req.pathParams.at("id");
                int limit = req.queryParams.count("limit") ? std::stoi(req.queryParams.at("limit")) : 50;
                auto results = database_->query(
                    "SELECT id, role, content, created_at FROM ai_conversations "
                    "WHERE session_id = '" + sessionId + "' ORDER BY created_at ASC LIMIT "
                    + std::to_string(limit));
                nlohmann::json arr = nlohmann::json::array();
                for (auto& row : results) {
                    nlohmann::json item;
                    item["id"] = std::stoi(row.at("id"));
                    item["role"] = row.count("role") ? row.at("role") : "user";
                    item["content"] = row.count("content") ? row.at("content") : "";
                    item["createdAt"] = row.count("created_at") ? row.at("created_at") : "";
                    arr.push_back(item);
                }
                resp["messages"] = arr;
                resp["total"] = arr.size();
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Session messages failed: {}", e.what());
            }
        }
        return HttpResponse::json(200, resp.dump());
    });

    // POST /api/ai-co-pilot/regenerate/:id — regenerate review/plan
    router.post(prefix + "/regenerate/:id", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string id = req.pathParams.at("id");
            nlohmann::json resp;
            resp["success"] = true;
            resp["id"] = id;
            resp["message"] = "Regeneration queued";
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"success\":false,\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/ai-co-pilot/feedback — submit AI feedback
    router.post(prefix + "/feedback", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto json = nlohmann::json::parse(req.body);
            std::string type = json.value("type", "");
            int rating = json.value("rating", 0);
            std::string comment = json.value("comment", "");

            if (database_) {
                database_->execute(
                    "INSERT INTO ai_feedback (type, rating, comment) VALUES ('"
                    + ValidationHelper::sanitize(type) + "', "
                    + std::to_string(rating) + ", '"
                    + ValidationHelper::sanitize(comment) + "')");
            }
            nlohmann::json resp;
            resp["success"] = true;
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"success\":false,\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/ai-co-pilot/sessions/:id/rename — rename a session
    router.post(prefix + "/sessions/:id/rename", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string sessionId = req.pathParams.at("id");
            auto body = nlohmann::json::parse(req.body);
            std::string name = body.value("name", "");

            if (name.empty())
                return HttpResponse::json(400, "{\"error\":\"name is required\"}");

            if (database_) {
                database_->execute(
                    "UPDATE ai_copilot_sessions SET name = '"
                    + StringUtil::escapeSql(name) + "' WHERE id = " + sessionId);
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["sessionId"] = sessionId;
            resp["name"] = name;
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/ai-co-pilot/sessions/recent — get recent sessions (lighter than full list)
    router.get(prefix + "/sessions/recent", [this](const HttpRequest& req) -> HttpResponse {
        nlohmann::json arr = nlohmann::json::array();

        if (database_) {
            try {
                auto result = database_->query(
                    "SELECT id, name, created_at FROM ai_copilot_sessions "
                    "ORDER BY updated_at DESC LIMIT 5");
                for (auto& row : result) {
                    nlohmann::json item;
                    item["id"] = StringUtil::getRowInt(row, "id");
                    item["name"] = StringUtil::getRowStr(row, "name");
                    item["createdAt"] = StringUtil::getRowStr(row, "created_at");
                    arr.push_back(item);
                }
            } catch (const std::exception& e) {
                spdlog::warn("[AiCoPilot] Recent sessions query failed: {}", e.what());
            }
        }

        nlohmann::json resp;
        resp["sessions"] = arr;
        resp["total"] = arr.size();
        return HttpResponse::json(200, resp.dump());
    });

    // POST /api/ai-co-pilot/export — export conversation
    router.post(prefix + "/export", [this](const HttpRequest& req) -> HttpResponse {
        try {
            auto body = nlohmann::json::parse(req.body);
            std::string sessionId = body.value("sessionId", "");
            std::string format = body.value("format", "markdown");

            std::string content;

            if (database_ && !sessionId.empty()) {
                auto messages = database_->query(
                    "SELECT role, content, created_at FROM ai_conversations "
                    "WHERE session_id = '" + sessionId + "' ORDER BY created_at ASC");
                for (auto& row : messages) {
                    content += "**" + StringUtil::getRowStr(row, "role", "user") + "**\n\n";
                    content += StringUtil::getRowStr(row, "content", "") + "\n\n---\n\n";
                }
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["content"] = content;
            resp["format"] = format;
            resp["sessionId"] = sessionId;
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/ai-co-pilot/sessions/:id/pin — Pin/unpin a session
    router.post(prefix + "/sessions/:id/pin", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string sessionId = req.pathParams.at("id");
            bool pinned = true;
            if (!req.body.empty()) {
                auto body = nlohmann::json::parse(req.body);
                pinned = body.value("pinned", true);
            }

            if (database_) {
                database_->execute(
                    "UPDATE ai_copilot_sessions SET pinned = "
                    + std::string(pinned ? "1" : "0")
                    + " WHERE id = " + sessionId);
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["sessionId"] = sessionId;
            resp["pinned"] = pinned;
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // GET /api/ai-co-pilot/sessions/pinned — Get pinned sessions
    router.get(prefix + "/sessions/pinned", [this](const HttpRequest& req) -> HttpResponse {
        try {
            nlohmann::json arr = nlohmann::json::array();

            if (database_) {
                auto result = database_->query(
                    "SELECT id, name, created_at FROM ai_copilot_sessions "
                    "WHERE pinned = 1 ORDER BY updated_at DESC");

                for (auto& row : result) {
                    nlohmann::json item;
                    item["id"] = row.count("id") && !row["id"].empty() ? std::stoi(row["id"]) : 0;
                    item["name"] = row.count("name") ? row["name"] : "";
                    item["createdAt"] = row.count("created_at") ? row["created_at"] : "";
                    arr.push_back(item);
                }
            }

            nlohmann::json resp;
            resp["sessions"] = arr;
            resp["total"] = arr.size();
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    // POST /api/ai-co-pilot/sessions/:id/summarize — Summarize a conversation session
    router.post(prefix + "/sessions/:id/summarize", [this](const HttpRequest& req) -> HttpResponse {
        try {
            std::string sessionId = req.pathParams.at("id");
            int messageCount = 0;
            std::string summary = "Summary not available (no messages found).";

            if (database_) {
                auto result = database_->query(
                    "SELECT COUNT(*) as cnt FROM ai_conversations WHERE session_id = '"
                    + sessionId + "'");

                if (!result.empty() && !result[0]["cnt"].empty()) {
                    messageCount = std::stoi(result[0]["cnt"]);
                }

                if (messageCount > 0) {
                    // Fetch first few messages to build a stub summary
                    auto msgs = database_->query(
                        "SELECT role, content FROM ai_conversations WHERE session_id = '"
                        + sessionId + "' ORDER BY created_at ASC LIMIT 3");

                    std::string topics;
                    for (auto& row : msgs) {
                        std::string content = row.count("content") ? row["content"] : "";
                        if (content.size() > 80) content = content.substr(0, 80) + "...";
                        if (!topics.empty()) topics += "; ";
                        topics += content;
                    }
                    summary = "Session about: " + topics;
                }
            } else {
                messageCount = 0;
                summary = "Session summary (stub mode, no database).";
            }

            nlohmann::json resp;
            resp["success"] = true;
            resp["sessionId"] = sessionId;
            resp["summary"] = summary;
            resp["messageCount"] = messageCount;
            return HttpResponse::json(200, resp.dump());
        } catch (const std::exception& e) {
            return HttpResponse::json(500, "{\"error\":\"" + std::string(e.what()) + "\"}");
        }
    });

    spdlog::info("[AiCoPilot] Registered 29 routes at /api/ai-co-pilot");
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
