/**
 * @file AiCoPilotModuleRoutes.cpp
 * @brief AI研究副驾驶模块路由实现
 *
 * 实现所有AI Co-Pilot API路由，包括：
 * - AI审稿人生成和查询
 * - 文献综述生成和查询
 * - 研究计划生成和查询
 * - 历史记录查询
 * - 统计和成本分析
 */

#include "business/AiCoPilotModule.hpp"
#include "core/Router.hpp"
#include "features/infrastructure/LoggingModule.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>

namespace PaperCrawler {

// ============================================================================
// 路由注册完整实现
// ============================================================================

void AiCoPilotModule::registerRoutes() {
    auto& router = Router::getInstance();
    std::string prefix = getRoutePrefix();

    // ================================================================
    // 1. AI审稿人系统
    // ================================================================

    // POST /api/ai-co-pilot/review - 生成AI审稿报告
    router.post(prefix + "/review", [this](const std::string& body, const std::map<std::string, std::string>& headers) {
        return handleGenerateReview(body);
    });

    // GET /api/ai-co-pilot/reviews/:userId - 获取审稿历史
    router.get(prefix + "/reviews/:userId", [this](const std::map<std::string, std::string>& params) {
        int userId = std::stoi(params.at("userId"));
        auto history = getReviewHistory(userId);
        return buildJsonResponse(history);
    });

    // ================================================================
    // 2. 文献综述生成器
    // ================================================================

    // POST /api/ai-co-pilot/literature-review/generate - 生成文献综述
    router.post(prefix + "/literature-review/generate", [this](const std::string& body, const std::map<std::string, std::string>& headers) {
        return handleGenerateLiteratureReview(body);
    });

    // GET /api/ai-co-pilot/literature-reviews - 获取文献综述列表
    router.get(prefix + "/literature-reviews", [this](const std::map<std::string, std::string>& params) {
        int userId = std::stoi(params.at("userId"));
        auto reviews = getLiteratureReviews(userId);
        return buildJsonResponse(reviews);
    });

    // ================================================================
    // 3. 研究规划助手
    // ================================================================

    // POST /api/ai-co-pilot/research-plan/generate - 生成研究计划
    router.post(prefix + "/research-plan/generate", [this](const std::string& body, const std::map<std::string, std::string>& headers) {
        return handleGenerateResearchPlan(body);
    });

    // GET /api/ai-co-pilot/research-plans - 获取研究计划列表
    router.get(prefix + "/research-plans", [this](const std::map<std::string, std::string>& params) {
        int userId = std::stoi(params.at("userId"));
        auto plans = getResearchPlans(userId);
        return buildJsonResponse(plans);
    });

    // ================================================================
    // 4. AI统计和成本分析
    // ================================================================

    // GET /api/ai-co-pilot/stats - 获取使用统计
    router.get(prefix + "/stats", [this](const std::map<std::string, std::string>& params) {
        int userId = std::stoi(params.at("userId"));
        auto stats = getUsageStats(userId);
        return buildJsonResponse(stats);
    });

    // GET /api/ai-co-pilot/costs - 获取成本统计
    router.get(prefix + "/costs", [this](const std::map<std::string, std::string>& params) {
        int userId = std::stoi(params.at("userId"));
        auto costs = getCostStats(userId);
        return buildJsonResponse(costs);
    });

    // ================================================================
    // 5. 对话助手（可选）
    // ================================================================

    // POST /api/ai-co-pilot/chat - AI对话
    router.post(prefix + "/chat", [this](const std::string& body, const std::map<std::string, std::string>& headers) {
        return handleChat(body);
    });

    // GET /api/ai-co-pilot/conversations - 获取对话列表
    router.get(prefix + "/conversations", [this](const std::map<std::string, std::string>& params) {
        int userId = std::stoi(params.at("userId"));
        auto conversations = getConversations(userId);
        return buildJsonResponse(conversations);
    });
}

// ============================================================================
// JSON响应构建辅助函数
// ============================================================================

std::string AiCoPilotModule::buildJsonResponse(const std::vector<AIReviewResult>& reviews) {
    nlohmann::json dataArr = nlohmann::json::array();

    for (const auto& review : reviews) {
        nlohmann::json item;
        item["id"] = review.paperId;
        item["type"] = "review";
        item["title"] = review.paperTitle;
        item["description"] = review.targetJournal + "审稿报告";
        item["status"] = "completed";
        item["timestamp"] = formatTimestamp(review.reviewedAt);
        item["duration"] = review.generationTimeMs / 1000;
        item["cost"] = review.estimatedCost;
        item["tokenCount"] = review.tokenCount;

        nlohmann::json inner;
        inner["reviewScore"] = review.reviewScore;
        inner["acceptanceProbability"] = review.acceptanceProbability;
        inner["methodologyScore"] = review.methodologyScore;
        inner["innovationScore"] = review.innovationScore;
        inner["presentationScore"] = review.presentationScore;
        inner["strengths"] = review.strengths;
        inner["weaknesses"] = review.weaknesses;
        item["data"] = inner;

        dataArr.push_back(item);
    }

    nlohmann::json response;
    response["success"] = true;
    response["data"] = dataArr;
    return response.dump();
}

std::string AiCoPilotModule::buildJsonResponse(const std::vector<LiteratureReviewResult>& reviews) {
    nlohmann::json dataArr = nlohmann::json::array();

    for (const auto& review : reviews) {
        nlohmann::json item;
        item["id"] = review.id;
        item["type"] = "literature-review";
        item["title"] = review.researchTopic;
        item["description"] = std::to_string(review.paperCount) + "篇论文的系统性综述";
        item["status"] = "completed";
        item["timestamp"] = formatTimestamp(review.generatedAt);
        item["duration"] = review.generationTimeMs / 1000;
        item["cost"] = review.estimatedCost;
        item["tokenCount"] = review.tokenCount;

        nlohmann::json inner;
        inner["paperCount"] = review.paperCount;
        inner["researchField"] = review.researchField;
        item["data"] = inner;

        dataArr.push_back(item);
    }

    nlohmann::json response;
    response["success"] = true;
    response["data"] = dataArr;
    return response.dump();
}

std::string AiCoPilotModule::buildJsonResponse(const std::vector<ResearchPlanResult>& plans) {
    nlohmann::json dataArr = nlohmann::json::array();

    for (const auto& plan : plans) {
        nlohmann::json item;
        item["id"] = plan.id;
        item["type"] = "research-plan";
        item["title"] = plan.projectTitle;
        item["description"] = std::to_string(plan.durationWeeks / 4) + "个月研究计划";
        item["status"] = "completed";
        item["timestamp"] = formatTimestamp(plan.generatedAt);
        item["duration"] = plan.generationTimeMs / 1000;
        item["cost"] = plan.estimatedCost;
        item["tokenCount"] = plan.tokenCount;

        nlohmann::json inner;
        inner["duration"] = plan.durationWeeks / 4;
        inner["feasibilityScore"] = plan.feasibilityScore;
        item["data"] = inner;

        dataArr.push_back(item);
    }

    nlohmann::json response;
    response["success"] = true;
    response["data"] = dataArr;
    return response.dump();
}

std::string AiCoPilotModule::buildJsonResponse(const std::map<std::string, std::string>& stats) {
    nlohmann::json data;
    for (const auto& [key, value] : stats) {
        // Try to parse as number; fall back to string
        try {
            double numVal = std::stod(value);
            // Check if it's actually an integer
            if (value.find('.') == std::string::npos) {
                data[key] = std::stoi(value);
            } else {
                data[key] = numVal;
            }
        } catch (...) {
            data[key] = value;
        }
    }

    nlohmann::json response;
    response["success"] = true;
    response["data"] = data;
    return response.dump();
}

std::string AiCoPilotModule::formatTimestamp(const std::chrono::system_clock::time_point& timePoint) {
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

std::string AiCoPilotModule::formatStringArray(const std::vector<std::string>& arr) {
    return nlohmann::json(arr).dump();
}

// ============================================================================
// HTTP处理器实现（返回mock数据用于前端测试）
// ============================================================================

std::string AiCoPilotModule::handleGenerateReview(const std::string& body) {
    // 解析请求（简化版）
    AIReviewRequest request;
    request.paperId = 1;
    request.userId = 1;
    request.targetJournal = "Nature";
    request.researchField = "Computer Science";
    request.includeComparison = true;
    request.reviewStyle = "balanced";

    // 生成mock响应
    auto result = generateReview(request);

    nlohmann::json inner;
    inner["reviewScore"] = result.reviewScore;
    inner["acceptanceProbability"] = result.acceptanceProbability;
    inner["methodologyScore"] = result.methodologyScore;
    inner["innovationScore"] = result.innovationScore;
    inner["presentationScore"] = result.presentationScore;
    inner["strengths"] = result.strengths;
    inner["weaknesses"] = result.weaknesses;

    nlohmann::json data;
    data["id"] = result.paperId;
    data["type"] = "review";
    data["title"] = "AI-Generated Paper Review";
    data["description"] = result.targetJournal + "审稿报告";
    data["status"] = "completed";
    data["timestamp"] = formatTimestamp(result.reviewedAt);
    data["duration"] = result.generationTimeMs / 1000;
    data["cost"] = result.estimatedCost;
    data["tokenCount"] = result.tokenCount;
    data["data"] = inner;

    nlohmann::json response;
    response["success"] = true;
    response["data"] = data;

    return response.dump();
}

std::string AiCoPilotModule::handleGenerateLiteratureReview(const std::string& body) {
    // 文献综述生成（前端对接完成后实现具体逻辑）
    nlohmann::json response;
    response["success"] = true;
    response["message"] = "Literature review generation started";
    return response.dump();
}

std::string AiCoPilotModule::handleGenerateResearchPlan(const std::string& body) {
    // 研究计划生成（前端对接完成后实现具体逻辑）
    nlohmann::json response;
    response["success"] = true;
    response["message"] = "Research plan generation started";
    return response.dump();
}

std::string AiCoPilotModule::handleChat(const std::string& body) {
    // AI对话（前端对接完成后实现具体逻辑）
    nlohmann::json response;
    response["success"] = true;
    response["response"] = "AI chat response";
    return response.dump();
}

// ============================================================================
// Mock数据生成（用于测试）
// ============================================================================

std::vector<AIReviewResult> AiCoPilotModule::getReviewHistory(int userId, int page, int limit) {
    std::vector<AIReviewResult> history;

    // 生成mock历史记录
    for (int i = 0; i < 5; ++i) {
        AIReviewResult result;
        result.paperId = 1000 + i;
        result.paperTitle = "Deep Learning for Computer Vision Applications " + std::to_string(i + 1);
        result.targetJournal = "Nature";
        result.reviewScore = 7 + (i % 3);
        result.acceptanceProbability = 0.6 + (i * 0.1);
        result.methodologyScore = 7 + (i % 3);
        result.innovationScore = 8 + (i % 2);
        result.presentationScore = 8 + (i % 2);
        result.strengths = {"Novel approach", "Good methodology", "Strong experimental results"};
        result.weaknesses = {"Limited experiments", "Missing comparison"};
        result.reviewedAt = std::chrono::system_clock::now() - std::chrono::hours(i * 24);
        result.generationTimeMs = 15000 + (i * 1000);
        result.estimatedCost = 0.0075;
        result.tokenCount = 2500;

        history.push_back(result);
    }

    return history;
}

std::map<std::string, std::string> AiCoPilotModule::getUsageStats(int userId) {
    std::map<std::string, std::string> stats;

    stats["totalGenerations"] = "127";
    stats["totalCost"] = "45.30";
    stats["monthlyCost"] = "18.50";
    stats["averageTime"] = "16";
    stats["successRate"] = "94";
    stats["growthRate"] = "23";
    stats["timeImprovement"] = "15";
    stats["successRateImprovement"] = "8";

    // 按类型统计
    stats["reviewCount"] = "58";
    stats["literatureReviewCount"] = "42";
    stats["researchPlanCount"] = "27";

    stats["costByReview"] = "18.25";
    stats["costByLiteratureReview"] = "16.80";
    stats["costByResearchPlan"] = "10.25";

    stats["averageReviewScore"] = "7.8";
    stats["averagePaperCount"] = "45";
    stats["averageFeasibility"] = "8.2";

    stats["totalTokens"] = "425000";
    stats["averageTokens"] = "3350";
    stats["costPerToken"] = "0.000107";

    return stats;
}

std::map<std::string, std::string> AiCoPilotModule::getCostStats(int userId) {
    std::map<std::string, std::string> costs;

    costs["totalCost"] = "45.30";
    costs["averageCostPerGeneration"] = "0.357";
    costs["mostExpensiveType"] = "review";
    costs["costTrend"] = "increasing";

    return costs;
}

} // namespace PaperCrawler
