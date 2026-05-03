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
#include "modules/LoggingModule.hpp"
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
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"data\": [\n";

    for (size_t i = 0; i < reviews.size(); ++i) {
        const auto& review = reviews[i];
        json << "    {\n";
        json << "      \"id\": " << review.paperId << ",\n";
        json << "      \"type\": \"review\",\n";
        json << "      \"title\": \"" << review.paperTitle << "\",\n";
        json << "      \"description\": \"" << review.targetJournal << "审稿报告\",\n";
        json << "      \"status\": \"completed\",\n";
        json << "      \"timestamp\": \"" << formatTimestamp(review.reviewedAt) << "\",\n";
        json << "      \"duration\": " << review.generationTimeMs / 1000 << ",\n";
        json << "      \"cost\": " << review.estimatedCost << ",\n";
        json << "      \"tokenCount\": " << review.tokenCount << ",\n";
        json << "      \"data\": {\n";
        json << "        \"reviewScore\": " << review.reviewScore << ",\n";
        json << "        \"acceptanceProbability\": " << review.acceptanceProbability << ",\n";
        json << "        \"methodologyScore\": " << review.methodologyScore << ",\n";
        json << "        \"innovationScore\": " << review.innovationScore << ",\n";
        json << "        \"presentationScore\": " << review.presentationScore << ",\n";
        json << "        \"strengths\": " << formatStringArray(review.strengths) << ",\n";
        json << "        \"weaknesses\": " << formatStringArray(review.weaknesses) << "\n";
        json << "      }\n";
        json << "    }";

        if (i < reviews.size() - 1) json << ",";
        json << "\n";
    }

    json << "  ]\n";
    json << "}";
    return json.str();
}

std::string AiCoPilotModule::buildJsonResponse(const std::vector<LiteratureReviewResult>& reviews) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"data\": [\n";

    for (size_t i = 0; i < reviews.size(); ++i) {
        const auto& review = reviews[i];
        json << "    {\n";
        json << "      \"id\": " << review.id << ",\n";
        json << "      \"type\": \"literature-review\",\n";
        json << "      \"title\": \"" << review.researchTopic << "\",\n";
        json << "      \"description\": \"" << review.paperCount << "篇论文的系统性综述\",\n";
        json << "      \"status\": \"completed\",\n";
        json << "      \"timestamp\": \"" << formatTimestamp(review.generatedAt) << "\",\n";
        json << "      \"duration\": " << review.generationTimeMs / 1000 << ",\n";
        json << "      \"cost\": " << review.estimatedCost << ",\n";
        json << "      \"tokenCount\": " << review.tokenCount << ",\n";
        json << "      \"data\": {\n";
        json << "        \"paperCount\": " << review.paperCount << ",\n";
        json << "        \"researchField\": \"" << review.researchField << "\"\n";
        json << "      }\n";
        json << "    }";

        if (i < reviews.size() - 1) json << ",";
        json << "\n";
    }

    json << "  ]\n";
    json << "}";
    return json.str();
}

std::string AiCoPilotModule::buildJsonResponse(const std::vector<ResearchPlanResult>& plans) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"data\": [\n";

    for (size_t i = 0; i < plans.size(); ++i) {
        const auto& plan = plans[i];
        json << "    {\n";
        json << "      \"id\": " << plan.id << ",\n";
        json << "      \"type\": \"research-plan\",\n";
        json << "      \"title\": \"" << plan.projectTitle << "\",\n";
        json << "      \"description\": \"" << plan.durationWeeks / 4 << "个月研究计划\",\n";
        json << "      \"status\": \"completed\",\n";
        json << "      \"timestamp\": \"" << formatTimestamp(plan.generatedAt) << "\",\n";
        json << "      \"duration\": " << plan.generationTimeMs / 1000 << ",\n";
        json << "      \"cost\": " << plan.estimatedCost << ",\n";
        json << "      \"tokenCount\": " << plan.tokenCount << ",\n";
        json << "      \"data\": {\n";
        json << "        \"duration\": " << plan.durationWeeks / 4 << ",\n";
        json << "        \"feasibilityScore\": " << plan.feasibilityScore << "\n";
        json << "      }\n";
        json << "    }";

        if (i < plans.size() - 1) json << ",";
        json << "\n";
    }

    json << "  ]\n";
    json << "}";
    return json.str();
}

std::string AiCoPilotModule::buildJsonResponse(const std::map<std::string, std::string>& stats) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"data\": {\n";

    size_t count = 0;
    for (const auto& [key, value] : stats) {
        json << "    \"" << key << "\": " << value;
        if (count < stats.size() - 1) json << ",";
        json << "\n";
        count++;
    }

    json << "  }\n";
    json << "}";
    return json.str();
}

std::string AiCoPilotModule::formatTimestamp(const std::chrono::system_clock::time_point& timePoint) {
    auto time = std::chrono::system_clock::to_time_t(timePoint);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

std::string AiCoPilotModule::formatStringArray(const std::vector<std::string>& arr) {
    std::ostringstream json;
    json << "[";
    for (size_t i = 0; i < arr.size(); ++i) {
        json << "\"" << arr[i] << "\"";
        if (i < arr.size() - 1) json << ", ";
    }
    json << "]";
    return json.str();
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

    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"data\": {\n";
    json << "    \"id\": " << result.paperId << ",\n";
    json << "    \"type\": \"review\",\n";
    json << "    \"title\": \"AI-Generated Paper Review\",\n";
    json << "    \"description\": \"" << result.targetJournal << "审稿报告\",\n";
    json << "    \"status\": \"completed\",\n";
    json << "    \"timestamp\": \"" << formatTimestamp(result.reviewedAt) << "\",\n";
    json << "    \"duration\": " << result.generationTimeMs / 1000 << ",\n";
    json << "    \"cost\": " << result.estimatedCost << ",\n";
    json << "    \"tokenCount\": " << result.tokenCount << ",\n";
    json << "    \"data\": {\n";
    json << "      \"reviewScore\": " << result.reviewScore << ",\n";
    json << "      \"acceptanceProbability\": " << result.acceptanceProbability << ",\n";
    json << "      \"methodologyScore\": " << result.methodologyScore << ",\n";
    json << "      \"innovationScore\": " << result.innovationScore << ",\n";
    json << "      \"presentationScore\": " << result.presentationScore << ",\n";
    json << "      \"strengths\": " << formatStringArray(result.strengths) << ",\n";
    json << "      \"weaknesses\": " << formatStringArray(result.weaknesses) << "\n";
    json << "    }\n";
    json << "  }\n";
    json << "}";

    return json.str();
}

std::string AiCoPilotModule::handleGenerateLiteratureReview(const std::string& body) {
    // 文献综述生成（前端对接完成后实现具体逻辑）
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"message\": \"Literature review generation started\"\n";
    json << "}";
    return json.str();
}

std::string AiCoPilotModule::handleGenerateResearchPlan(const std::string& body) {
    // 研究计划生成（前端对接完成后实现具体逻辑）
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"message\": \"Research plan generation started\"\n";
    json << "}";
    return json.str();
}

std::string AiCoPilotModule::handleChat(const std::string& body) {
    // AI对话（前端对接完成后实现具体逻辑）
    std::ostringstream json;
    json << "{\n";
    json << "  \"success\": true,\n";
    json << "  \"response\": \"AI chat response\"\n";
    json << "}";
    return json.str();
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
