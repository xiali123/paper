#pragma once

#include "core/IModule.hpp"
#include "core/ModuleExports.hpp"
#include "data/IDatabase.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <chrono>
#include <set>

namespace PaperCrawler {

// 前向声明
namespace Network {
    class HttpClient;
}

using HttpClientPtr = std::shared_ptr<Network::HttpClient>;

/**
 * @brief AI请求配置
 */
struct AiConfig {
    std::string provider{"openai"};  // openai, anthropic, local
    std::string apiKey;
    std::string baseUrl{"https://api.openai.com/v1"};
    std::string model{"gpt-3.5-turbo"};
    double temperature{0.7};
    int maxTokens{1000};
    int timeoutSeconds{30};
};

/**
 * @brief 论文摘要请求
 */
struct PaperSummaryRequest {
    int paperId;
    std::string language{"zh"};  // zh, en
    int maxLength{500};          // 摘要最大长度
    bool includeKeywords{true};
    bool includeContributions{true};
};

/**
 * @brief 论文摘要结果
 */
struct PaperSummaryResult {
    int paperId;
    std::string title;
    std::string summary;
    std::vector<std::string> keywords;
    std::vector<std::string> contributions;
    std::string language;
    std::chrono::system_clock::time_point generatedAt;
};

/**
 * @brief 问答请求
 */
struct QuestionRequest {
    int paperId;                      // 论文ID
    std::string question;             // 问题
    std::string context;              // 额外上下文（可选）
    std::string language{"zh"};       // 回答语言
    bool includeReferences{true};     // 是否引用原文
};

/**
 * @brief 问答结果
 */
struct QuestionResult {
    int paperId;
    std::string question;
    std::string answer;
    std::vector<std::string> references;  // 引用的原文段落
    std::string language;
    double confidence{0.0};
    std::chrono::system_clock::time_point answeredAt;
};

/**
 * @brief AI API模块
 *
 * 功能：
 * 1. 论文摘要生成
 * 2. 智能问答
 * 3. 关键词提取
 * 4. 贡献点总结
 * 5. 多语言支持（中文、英文）
 */
class AiApiModule : public IModule {
public:
    AiApiModule();
    ~AiApiModule() override;

    std::string getName() const override { return "AI"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override {
        return "AI-powered paper summarization and Q&A";
    }
    ModuleType getModuleType() const override { return ModuleType::BUSINESS; }
    std::string getRoutePrefix() const override { return "/api/ai"; }

    bool initialize() override;
    bool start() override;
    bool stop() override;
    void cleanup() override;

    /**
     * @brief 设置AI配置
     */
    void setConfig(const AiConfig& config);

    /**
     * @brief 获取配置
     */
    AiConfig getConfig() const;

    // ========================================================================
    // API端点处理方法
    // ========================================================================

    /**
     * @brief 生成论文摘要
     * POST /api/ai/summary
     */
    std::string generatePaperSummary(const PaperSummaryRequest& request);

    /**
     * @brief 批量生成论文摘要
     * POST /api/ai/summary/batch
     */
    std::vector<PaperSummaryResult> generateBatchSummaries(
        const std::vector<int>& paperIds,
        const std::string& language = "zh",
        int maxLength = 500
    );

    /**
     * @brief 论文问答
     * POST /api/ai/question
     */
    std::string askQuestion(const QuestionRequest& request);

    /**
     * @brief 提取关键词
     * GET /api/ai/keywords/:paperId
     */
    std::vector<std::string> extractKeywords(int paperId, int count = 10);

    /**
     * @brief 总结贡献点
     * GET /api/ai/contributions/:paperId
     */
    std::vector<std::string> summarizeContributions(int paperId);

    /**
     * @brief 比较多篇论文
     * POST /api/ai/compare
     */
    std::string comparePapers(const std::vector<int>& paperIds);

    /**
     * @brief 获取AI统计信息
     * GET /api/ai/stats
     */
    std::map<std::string, std::string> getStats();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    // ========================================================================
    // 内部辅助方法
    // ========================================================================

    /**
     * @brief 从数据库获取论文内容
     */
    std::optional<std::string> fetchPaperContent(int paperId);

    /**
     * @brief 调用AI API生成摘要
     */
    std::string callAiApiForSummary(
        const std::string& title,
        const std::string& abstract,
        const std::string& content,
        const std::string& language,
        int maxLength
    );

    /**
     * @brief 调用AI API回答问题
     */
    std::string callAiApiForQuestion(
        const std::string& paperContent,
        const std::string& question,
        const std::string& language
    );

    /**
     * @brief 构建摘要生成提示词
     */
    std::string buildSummaryPrompt(
        const std::string& title,
        const std::string& abstract,
        const std::string& language,
        int maxLength
    );

    /**
     * @brief 构建问答提示词
     */
    std::string buildQuestionPrompt(
        const std::string& paperContent,
        const std::string& question,
        const std::string& language
    );

    /**
     * @brief 解析AI响应
     */
    PaperSummaryResult parseSummaryResponse(
        int paperId,
        const std::string& title,
        const std::string& aiResponse,
        const std::string& language
    );

    /**
     * @brief 缓存AI结果
     */
    void cacheAiResult(const std::string& key, const std::string& result);

    /**
     * @brief 从缓存获取AI结果
     */
    std::optional<std::string> getCachedResult(const std::string& key);
};

} // namespace PaperCrawler
