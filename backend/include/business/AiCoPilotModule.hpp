#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

namespace PaperCrawler {

/**
 * @brief AI审稿请求
 */
struct AIReviewRequest {
    int paperId;
    int userId;
    std::string targetJournal;  // 目标期刊
    bool includeComparison;     // 是否包含对比分析
    std::string reviewStyle;    // 审稿风格：strict, balanced, encouraging
};

/**
 * @brief AI审稿结果
 */
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

/**
 * @brief 文献综述生成请求
 */
struct LiteratureReviewRequest {
    int userId;
    std::string title;
    std::string researchField;
    std::vector<int> paperIds;
    int maxLength;  // 最大字数
    bool includeGaps;       // 包含研究空白
    bool includeTrends;     // 包含研究趋势
    bool includeMethodology; // 包含方法论总结
};

/**
 * @brief 文献综述结果
 */
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

/**
 * @brief 研究规划请求
 */
struct ResearchPlanRequest {
    int userId;
    std::string title;
    std::string researchQuestion;
    std::string researchField;
    std::vector<std::string> keywords;
    int durationMonths;
    std::string budgetLevel;  // low, medium, high
};

/**
 * @brief 研究规划结果
 */
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
    std::vector<std::string> expectedOutcomes; // 预期成果
    int feasibilityScore;  // 可行性评分 1-10
    int innovationScore;   // 创新性评分 1-10
    std::string impactPrediction; // 影响力预测
    double costUsd;
    std::chrono::system_clock::time_point plannedAt;
};

/**
 * @brief AI研究副驾驶模块
 *
 * 功能：
 * 1. AI审稿人系统 - 模拟顶级期刊审稿流程
 * 2. AI文献综述生成器 - 自动生成领域综述
 * 3. AI研究规划助手 - 生成研究计划书
 * 4. 多轮对话式智能助手 - 理解用户研究背景
 *
 * ROI: 9.5/10
 * 开发周期: 3-4周
 */
class AiCoPilotModule : public BusinessModuleBase {
public:
    explicit AiCoPilotModule(std::shared_ptr<IDatabase> database);
    ~AiCoPilotModule() override;

    std::string getName() const override { return "AiCoPilot"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "AI Research Co-Pilot with review, literature review, and planning";
    }

    // ========================================================================
    // 1. AI审稿人系统
    // ========================================================================

    /**
     * @brief 生成AI审稿报告
     * POST /api/ai-co-pilot/review
     */
    AIReviewResult generateReview(const AIReviewRequest& request);

    /**
     * @brief 获取审稿历史
     * GET /api/ai-co-pilot/reviews/:userId
     */
    std::vector<AIReviewResult> getReviewHistory(int userId, int page = 1, int limit = 20);

    /**
     * @brief 获取特定审稿报告
     * GET /api/ai-co-pilot/review/:id
     */
    std::optional<AIReviewResult> getReview(int reviewId);

    // ========================================================================
    // 2. AI文献综述生成器
    // ========================================================================

    /**
     * @brief 生成文献综述
     * POST /api/ai-co-pilot/literature-review/generate
     */
    LiteratureReviewResult generateLiteratureReview(const LiteratureReviewRequest& request);

    /**
     * @brief 获取用户的文献综述列表
     * GET /api/ai-co-pilot/literature-reviews
     */
    std::vector<LiteratureReviewResult> getLiteratureReviews(int userId, int page = 1, int limit = 20);

    /**
     * @brief 获取特定文献综述
     * GET /api/ai-co-pilot/literature-review/:id
     */
    std::optional<LiteratureReviewResult> getLiteratureReview(int reviewId);

    /**
     * @brief 更新文献综述
     * PUT /api/ai-co-pilot/literature-review/:id
     */
    bool updateLiteratureReview(int reviewId, const std::string& updatedContent);

    // ========================================================================
    // 3. AI研究规划助手
    // ========================================================================

    /**
     * @brief 生成研究计划
     * POST /api/ai-co-pilot/research-plan/generate
     */
    ResearchPlanResult generateResearchPlan(const ResearchPlanRequest& request);

    /**
     * @brief 获取用户的研究计划列表
     * GET /api/ai-co-pilot/research-plans
     */
    std::vector<ResearchPlanResult> getResearchPlans(int userId, int page = 1, int limit = 20);

    /**
     * @brief 获取特定研究计划
     * GET /api/ai-co-pilot/research-plan/:id
     */
    std::optional<ResearchPlanResult> getResearchPlan(int planId);

    // ========================================================================
    // 4. 多轮对话式智能助手
    // ========================================================================

    /**
     * @brief 发送对话消息
     * POST /api/ai-co-pilot/chat
     */
    std::string chat(int userId, const std::string& message, const std::string& sessionId = "");

    /**
     * @brief 获取对话历史
     * GET /api/ai-co-pilot/conversations
     */
    std::vector<std::map<std::string, std::string>> getConversations(int userId);

    /**
     * @brief 获取特定对话
     * GET /api/ai-co-pilot/conversation/:sessionId
     */
    std::vector<std::map<std::string, std::string>> getConversation(const std::string& sessionId);

    // ========================================================================
    // 5. AI研究建议
    // ========================================================================

    /**
     * @brief 获取个性化研究建议
     * GET /api/ai-co-pilot/recommendations
     */
    std::vector<std::map<std::string, std::string>> getRecommendations(int userId);

    /**
     * @brief 标记建议为已完成
     * PUT /api/ai-co-pilot/recommendations/:id/complete
     */
    bool completeRecommendation(int recommendationId);

    /**
     * @brief 忽略建议
     * PUT /api/ai-co-pilot/recommendations/:id/dismiss
     */
    bool dismissRecommendation(int recommendationId);

    // ========================================================================
    // 6. 统计和分析
    // ========================================================================

    /**
     * @brief 获取AI使用统计
     * GET /api/ai-co-pilot/stats
     */
    std::map<std::string, std::string> getUsageStats(int userId);

    /**
     * @brief 获取成本统计
     * GET /api/ai-co-pilot/costs
     */
    std::map<std::string, std::string> getCostStats(int userId);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    std::shared_ptr<IDatabase> database_;
    std::shared_ptr<UnifiedAIWorkflow> aiWorkflow_;

    void registerRoutes() override;

    // HTTP处理器
    std::string handleGenerateReview(const std::string& body);
    std::string handleGenerateLiteratureReview(const std::string& body);
    std::string handleGenerateResearchPlan(const std::string& body);
    std::string handleChat(const std::string& body);
    std::string handleGetRecommendations(const std::map<std::string, std::string>& params);
    std::string handleGetStats(const std::map<std::string, std::string>& params);

    // 辅助方法
    std::string buildReviewPrompt(const AIReviewRequest& request, const std::map<std::string, std::string>& paperData);
    std::string buildLiteratureReviewPrompt(const LiteratureReviewRequest& request);
    std::string buildResearchPlanPrompt(const ResearchPlanRequest& request);
};

} // namespace PaperCrawler
