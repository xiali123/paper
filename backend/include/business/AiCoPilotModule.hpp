#pragma once

#include "core/ModuleBase.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include "business/UnifiedAIWorkflow.hpp"
#include "prompts/AIPromptTemplates.hpp"
#include "business/AIResponseParser.hpp"
#include "network/SseConnection.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <thread>
#include <atomic>

namespace PaperCrawler {

// 前向声明
using AI::ReviewPromptContext;
using AI::ReviewPromptStyle;
using AI::LiteratureReviewContext;
using AI::ResearchPlanContext;
using AI::AIReviewResult;
using AI::LiteratureReviewResult;
using AI::ResearchPlanResult;

/**
 * @brief AI审稿请求
 */
struct AIReviewRequest {
    int paperId;
    int userId;
    std::string targetJournal;  // 目标期刊
    std::string researchField;  // 研究领域（新增）
    bool includeComparison;     // 是否包含对比分析
    std::string reviewStyle;    // 审稿风格：strict, balanced, encouraging
};

// 使用AIResponseParser中定义的AIReviewResult
// 使用AIResponseParser中定义的LiteratureReviewResult
// 使用AIResponseParser中定义的ResearchPlanResult


// 使用AIResponseParser中定义的LiteratureReviewRequest
// 使用AIResponseParser中定义的ResearchPlanRequest


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

    // SSE streaming support
    SseBroadcaster sseBroadcaster_;
    std::atomic<uint64_t> activeStreamCount_{0};
    std::atomic<uint64_t> totalStreamedRequests_{0};

    void registerRoutes() override;

    // HTTP处理器
    std::string handleGenerateReview(const std::string& body);
    std::string handleGenerateLiteratureReview(const std::string& body);
    std::string handleGenerateResearchPlan(const std::string& body);
    std::string handleChat(const std::string& body);
    std::string handleGetRecommendations(const std::map<std::string, std::string>& params);
    std::string handleGetStats(const std::map<std::string, std::string>& params);

    // SSE streaming handlers
    HttpResponse handleStreamRequest(const HttpRequest& req);
    HttpResponse handleStreamStatus(const HttpRequest& req);

    // SSE helper: chunk a full AI response into SSE event fragments written to sseBody
    void streamAiResponseInto(const std::string& connectionId,
                              const std::string& fullResponse,
                              std::ostringstream& sseBody);

    // 辅助方法
    std::string buildReviewPrompt(const AIReviewRequest& request, const std::map<std::string, std::string>& paperData);
    std::string buildLiteratureReviewPrompt(const LiteratureReviewRequest& request);
    std::string buildResearchPlanPrompt(const ResearchPlanRequest& request);
};

} // namespace PaperCrawler
