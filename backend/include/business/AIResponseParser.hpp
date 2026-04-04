/**
 * AI Response Parser
 * AI响应解析器 - 头文件
 *
 * 文件位置: backend/include/business/AIResponseParser.hpp
 * 创建时间: 2026-04-04
 * 作者: PaperCrawler Team
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace PaperCrawler {

using json = nlohmann::json;

// ============================================================================
// 前向声明
// ============================================================================

// AI审稿请求（已存在于AiCoPilotModule.hpp中）
struct AIReviewRequest;

// 文献综述请求（已存在于AiCoPilotModule.hpp中）
struct LiteratureReviewRequest;

// 研究规划请求（已存在于AiCoPilotModule.hpp中）
struct ResearchPlanRequest;

// AI审稿结果（已存在于AiCoPilotModule.hpp中）
struct AIReviewResult {
    bool success;
    int paperId;
    int reviewScore;            // 1-10分
    float acceptanceProbability; // 录用概率 0-1
    std::vector<std::string> strengths;     // 论文亮点
    std::vector<std::string> weaknesses;    // 论文不足
    std::vector<std::string> improvements;  // 改进建议
    std::map<std::string, std::string> comparedPapers; // 对比论文
    std::string reviewerComments; // 审稿意见
    double costUsd;
    std::chrono::system_clock::time_point reviewedAt;
};

// 文献综述结果（已存在于AiCoPilotModule.hpp中）
struct LiteratureReviewResult {
    bool success;
    int reviewId;
    std::string title;
    std::string reviewContent;     // 综述内容
    std::vector<std::string> researchGaps;   // 研究空白
    std::vector<std::string> trends;         // 研究趋势
    std::string methodologySummary; // 方法论总结
    std::vector<std::string> keyFindings;    // 主要发现
    std::vector<std::string> futureDirections; // 未来方向
    int paperCount;
    double costUsd;
    std::chrono::system_clock::time_point generatedAt;
};

// 研究规划结果（已存在于AiCoPilotModule.hpp中）
struct ResearchPlanResult {
    bool success;
    int planId;
    std::string title;
    std::string researchQuestion;
    std::vector<std::string> objectives;    // 研究目标
    std::string methodology;                // 方法论
    std::map<std::string, std::string> timeline; // 时间安排
    std::vector<std::string> requiredResources; // 所需资源
    std::vector<std::string> potentialChallenges; // 潜在挑战
    std::vector<std::string> expectedOutcomes;   // 预期成果
    int feasibilityScore;        // 可行性评分 1-10
    int innovationScore;         // 创新性评分 1-10
    int impactScore;             // 影响力评分 1-10
    double costUsd;
    std::chrono::system_clock::time_point plannedAt;
};

// ============================================================================
// AI响应解析器类
// ============================================================================

/**
 * @brief AI响应解析器
 *
 * 功能：
 * 1. 解析AI审稿人JSON响应
 * 2. 解析文献综述JSON响应
 * 3. 解析研究计划JSON响应
 * 4. 容错解析（Fallback）
 * 5. 数据验证和清洗
 */
class AIResponseParser {
public:
    /**
     * @brief 解析AI审稿人响应
     * @param jsonResponse AI返回的JSON字符串
     * @param paperId 论文ID
     * @param userId 用户ID
     * @return 解析后的审稿结果
     */
    static AIReviewResult parseReviewResponse(
        const std::string& jsonResponse,
        int paperId,
        int userId
    );

    /**
     * @brief 解析文献综述响应
     * @param jsonResponse AI返回的JSON字符串
     * @param request 原始请求
     * @return 解析后的综述结果
     */
    static LiteratureReviewResult parseLiteratureReviewResponse(
        const std::string& jsonResponse,
        const LiteratureReviewRequest& request
    );

    /**
     * @brief 解析研究计划响应
     * @param jsonResponse AI返回的JSON字符串
     * @param request 原始请求
     * @return 解析后的计划结果
     */
    static ResearchPlanResult parseResearchPlanResponse(
        const std::string& jsonResponse,
        const ResearchPlanRequest& request
    );

private:
    // 容错解析方法
    static AIReviewResult parseReviewResponseFallback(
        const std::string& response,
        int paperId,
        int userId
    );

    static LiteratureReviewResult parseLiteratureReviewResponseFallback(
        const std::string& response,
        const LiteratureReviewRequest& request
    );

    static ResearchPlanResult parseResearchPlanResponseFallback(
        const std::string& response,
        const ResearchPlanRequest& request
    );

    // JSON验证方法
    static bool validateReviewJson(const nlohmann::json& j);
    static bool validateLiteratureReviewJson(const nlohmann::json& j);
    static bool validateResearchPlanJson(const nlohmann::json& j);

    // 辅助方法
    static std::vector<std::string> splitText(const std::string& text);
    static std::string extractSection(const std::string& text, const std::string& sectionName);
};

} // namespace PaperCrawler
