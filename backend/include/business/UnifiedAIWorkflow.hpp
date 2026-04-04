#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
// #include "modules/CacheModule.hpp"  // TODO: CacheModule not implemented yet
#include "network/HttpClient.hpp"
#include <string>
#include <map>
#include <memory>
#include <optional>
#include <functional>
#include <chrono>

namespace PaperCrawler {

/**
 * @brief AI缓存级别
 */
enum class AICacheLevel {
    L1_MEMORY,   // 内存缓存（最近查询，30%命中率）
    L2_REDIS,    // Redis缓存（热门查询，50%命中率）
    L3_PRECOMPUTED // 预计算缓存（批量任务，15%命中率）
};

/**
 * @brief AI模型类型
 */
enum class AIModelType {
    GPT_4,           // 高质量，成本高
    GPT_4_MINI,      // 性价比高
    LOCAL_MODEL,     // 本地模型（免费）
    CLAUDE_3_5_SONNET // Claude模型
};

/**
 * @brief AI请求结果
 */
struct AIResult {
    bool success;
    std::string content;
    std::string errorMessage;
    int tokensUsed;
    double costUsd;
    std::string model;
    std::chrono::system_clock::time_point timestamp;
    bool fromCache;
};

/**
 * @brief RAG（检索增强生成）上下文
 */
struct RAGContext {
    std::vector<std::string> relevantPapers;  // 相关论文ID
    std::vector<std::string> knowledgeGraphEntities; // 知识图谱实体
    std::map<std::string, std::string> userContext; // 用户上下文
    std::vector<std::string> conversationHistory; // 对话历史
};

/**
 * @brief 统一AI工作流引擎
 *
 * 功能：
 * 1. RAG架构（检索增强生成）
 * 2. 三层AI缓存（L1内存 + L2 Redis + L3预计算）
 * 3. 成本优化（本地模型 + 云端API混合）
 * 4. 批量处理优化（OpenAI Batch API）
 * 5. 流式响应（SSE/WebSocket）
 */
class UnifiedAIWorkflow : public IModule {
public:
    UnifiedAIWorkflow();
    ~UnifiedAIWorkflow() override;

    // IModule接口实现
    std::string getName() const override { return "UnifiedAIWorkflow"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "Unified AI workflow with RAG and multi-level caching";
    }
    ModuleType getModuleType() const override { return ModuleType::SERVER; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    // ========================================================================
    // 核心AI工作流
    // ========================================================================

    /**
     * @brief 执行AI请求（带RAG和缓存）
     *
     * @param prompt 用户提示词
     * @param model 模型类型
     * @param ragContext RAG上下文（可选）
     * @param userId 用户ID（用于个性化）
     * @return AIResult AI响应结果
     */
    AIResult executeAIRequest(
        const std::string& prompt,
        AIModelType model = AIModelType::GPT_4_MINI,
        const std::optional<RAGContext>& ragContext = std::nullopt,
        int userId = 0
    );

    /**
     * @brief 批量AI请求（使用OpenAI Batch API，50%折扣）
     */
    std::vector<AIResult> executeBatchRequest(
        const std::vector<std::string>& prompts,
        AIModelType model = AIModelType::GPT_4_MINI
    );

    /**
     * @brief 流式AI响应（SSE/WebSocket）
     */
    void streamAIResponse(
        const std::string& prompt,
        std::function<void(const std::string& chunk)> callback,
        AIModelType model = AIModelType::GPT_4_MINI
    );

    // ========================================================================
    // RAG（检索增强生成）功能
    // ========================================================================

    /**
     * @brief 构建RAG上下文
     *
     * @param query 用户查询
     * @param userId 用户ID
     * @return RAGContext RAG上下文
     */
    RAGContext buildRAGContext(const std::string& query, int userId);

    /**
     * @brief 增强Prompt（添加检索到的上下文）
     */
    std::string enhancePromptWithContext(
        const std::string& originalPrompt,
        const RAGContext& context
    );

    // ========================================================================
    // 缓存管理
    // ========================================================================

    /**
     * @brief 检查缓存
     */
    std::optional<AIResult> checkCache(const std::string& cacheKey);

    /**
     * @brief 保存到缓存
     */
    void saveToCache(const std::string& cacheKey, const AIResult& result);

    /**
     * @brief 生成缓存键
     */
    std::string generateCacheKey(
        const std::string& prompt,
        AIModelType model,
        const std::optional<RAGContext>& context
    );

    /**
     * @brief 获取缓存统计
     */
    std::map<std::string, std::string> getCacheStats();

    // ========================================================================
    // 成本优化
    // ========================================================================

    /**
     * @brief 选择最优模型（基于任务复杂度和预算）
     */
    AIModelType selectOptimalModel(const std::string& task, int complexity);

    /**
     * @brief 计算请求成本
     */
    double calculateCost(AIModelType model, int inputTokens, int outputTokens);

    /**
     * @brief 获取总成本统计
     */
    std::map<std::string, std::string> getCostStats();

    // ========================================================================
    // 流式响应
    // ========================================================================

    /**
     * @brief 启用流式响应
     */
    void enableStreaming(bool enabled);

    /**
     * @brief 设置流式回调
     */
    void setStreamingCallback(std::function<void(const std::string&)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // ========================================================================
    // 内部辅助方法
    // ========================================================================

    /**
     * @brief 调用AI API（云端）
     */
    AIResult callCloudAPI(const std::string& prompt, AIModelType model);

    /**
     * @brief 调用本地模型
     */
    AIResult callLocalModel(const std::string& prompt);

    /**
     * @brief 调用OpenAI API
     */
    std::string callOpenAIAPI(const std::string& prompt, const std::string& model);

    /**
     * @brief 调用Claude API
     */
    std::string callClaudeAPI(const std::string& prompt, const std::string& model);

    /**
     * @brief 构建完整Prompt（包含RAG上下文）
     */
    std::string buildFullPrompt(
        const std::string& userPrompt,
        const RAGContext& context
    );

    /**
     * @brief 解析AI响应
     */
    AIResult parseAIResponse(
        const std::string& response,
        AIModelType model,
        bool fromCache
    );

    /**
     * @brief 预计算并缓存常见查询
     */
    void precomputeCommonQueries();

    /**
     * @brief 智能模型选择（基于内容分析）
     */
    AIModelType intelligentModelSelection(const std::string& content);
};

} // namespace PaperCrawler
