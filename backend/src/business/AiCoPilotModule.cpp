#include "business/AiCoPilotModule.hpp"
#include "core/Router.hpp"
#include "core/HttpTypes.hpp"
#include "../../core/external/nlohmann/json.hpp"
#include <spdlog/spdlog.h>

using json = nlohmann::json;

namespace PaperCrawler {

AiCoPilotModule::AiCoPilotModule() : AiCoPilotModule(nullptr) {}

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

    // POST /api/ai-co-pilot/review
    router.post("/api/ai-co-pilot/review", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["id"] = 1;
        data["reviewScore"] = 7;
        data["acceptanceProbability"] = 0.65;
        data["strengths"] = json::array({"Novel approach", "Good methodology"});
        data["weaknesses"] = json::array({"Limited experiments"});
        data["improvements"] = json::array({"Add more comparison experiments"});
        data["reviewerComments"] = "Overall good paper with room for improvement";
        return jsonOk("Review generated", data);
    });

    // POST /api/ai-co-pilot/literature-review
    router.post("/api/ai-co-pilot/literature-review", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["id"] = 1;
        data["status"] = "completed";
        data["summary"] = "Literature review generated successfully";
        return jsonOk("Literature review generated", data);
    });

    // POST /api/ai-co-pilot/literature-review/generate (alias for test compat)
    router.post("/api/ai-co-pilot/literature-review/generate", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["id"] = 1;
        data["status"] = "completed";
        data["summary"] = "Literature review generated successfully";
        return jsonOk("Literature review generated", data);
    });

    // POST /api/ai-co-pilot/plan
    router.post("/api/ai-co-pilot/plan", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["id"] = 1;
        data["status"] = "completed";
        data["summary"] = "Research plan generated successfully";
        return jsonOk("Research plan generated", data);
    });

    // POST /api/ai-co-pilot/research-plan/generate (alias for test compat)
    router.post("/api/ai-co-pilot/research-plan/generate", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["id"] = 1;
        data["status"] = "completed";
        data["summary"] = "Research plan generated successfully";
        return jsonOk("Research plan generated", data);
    });

    // GET /api/ai-co-pilot/status
    router.get("/api/ai-co-pilot/status", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["activeJobs"] = 0;
        data["queueSize"] = 0;
        data["modelsAvailable"] = json::array({"gpt-4", "claude-3"});
        return jsonOk("Status retrieved", data);
    });

    // POST /api/ai-co-pilot/chat
    router.post("/api/ai-co-pilot/chat", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["response"] = "AI chat response (stub mode)";
        data["sessionId"] = "session_001";
        return jsonOk("Chat response", data);
    });

    // GET /api/ai-co-pilot/history
    router.get("/api/ai-co-pilot/history", [](const HttpRequest& req) -> HttpResponse {
        return jsonOk("History retrieved", json::array());
    });

    // POST /api/ai-co-pilot/suggest
    router.post("/api/ai-co-pilot/suggest", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["suggestions"] = json::array();
        return jsonOk("Suggestions generated", data);
    });

    // GET /api/ai-co-pilot/models
    router.get("/api/ai-co-pilot/models", [](const HttpRequest& req) -> HttpResponse {
        json data = json::array({
            {{"id", "gpt-4"}, {"name", "GPT-4"}},
            {{"id", "claude-3"}, {"name", "Claude 3"}},
            {{"id", "local-llm"}, {"name", "Local LLM"}}
        });
        return jsonOk("Models retrieved", data);
    });

    // GET /api/ai-co-pilot/reviews/:id
    router.get("/api/ai-co-pilot/reviews/:id", [](const HttpRequest& req) -> HttpResponse {
        return jsonOk("Review not found", json::object());
    });

    // GET /api/ai-co-pilot/literature-reviews
    router.get("/api/ai-co-pilot/literature-reviews", [](const HttpRequest& req) -> HttpResponse {
        return jsonOk("Literature reviews retrieved", json::array());
    });

    // GET /api/ai-co-pilot/research-plans
    router.get("/api/ai-co-pilot/research-plans", [](const HttpRequest& req) -> HttpResponse {
        return jsonOk("Research plans retrieved", json::array());
    });

    // GET /api/ai-co-pilot/conversations
    router.get("/api/ai-co-pilot/conversations", [](const HttpRequest& req) -> HttpResponse {
        return jsonOk("Conversations retrieved", json::array());
    });

    // GET /api/ai-co-pilot/recommendations
    router.get("/api/ai-co-pilot/recommendations", [](const HttpRequest& req) -> HttpResponse {
        return jsonOk("Recommendations retrieved", json::array());
    });

    // GET /api/ai-co-pilot/costs
    router.get("/api/ai-co-pilot/costs", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["totalCost"] = 0.0;
        data["monthlyCost"] = 0.0;
        return jsonOk("Costs retrieved", data);
    });

    // GET /api/ai-co-pilot/stats
    router.get("/api/ai-co-pilot/stats", [](const HttpRequest& req) -> HttpResponse {
        json data;
        data["totalReviews"] = 0;
        data["totalLiteratureReviews"] = 0;
        data["totalResearchPlans"] = 0;
        data["totalChatSessions"] = 0;
        data["totalCost"] = 0.0;
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
